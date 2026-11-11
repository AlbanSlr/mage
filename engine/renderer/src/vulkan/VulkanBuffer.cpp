#include "vulkan/VulkanBuffer.hpp"

#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"

#include <mage/core/Assert.hpp>

#include <vk_mem_alloc.h>

#include <cstddef>
#include <cstring>
#include <format>
#include <span>
#include <stdexcept>
#include <utility>

namespace mage::renderer
{
    VulkanBuffer::VulkanBuffer(const VulkanAllocator& allocator, const VulkanBufferDescription& description)
        : m_allocator(allocator.handle())
        , m_sizeInBytes(description.sizeInBytes)
    {
        MAGE_ASSERT(description.sizeInBytes > 0, "A buffer cannot be empty");

        const vk::BufferCreateInfo bufferCreateInfo{
            .size = description.sizeInBytes,
            .usage = description.usage,

            // Exclusive: only used by queues of a single family.
            .sharingMode = vk::SharingMode::eExclusive,
        };

        // "Auto" lets VMA choose the memory type from the buffer usage and
        // the flags below.
        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        if (description.memoryAccess == BufferMemoryAccess::CpuWritable)
        {
            // HOST_ACCESS_SEQUENTIAL_WRITE: the CPU writes to this memory and
            // never reads it back, which allows VMA to pick write-combined
            // memory (fast to fill, very slow to read).
            // MAPPED: keep the memory permanently mapped into the CPU address
            // space, so writing to it is a plain memcpy.
            allocationCreateInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        // Vulkan-Hpp structures convert to their C equivalent, which is what
        // the C API of VMA expects.
        const VkBufferCreateInfo& nativeBufferCreateInfo = bufferCreateInfo;

        VmaAllocationInfo allocationInfo{};
        const auto        result = static_cast<vk::Result>(vmaCreateBuffer(
            m_allocator, &nativeBufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_allocation, &allocationInfo));
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(std::format(
                "Failed to create a buffer of {} bytes: {}", description.sizeInBytes, vk::to_string(result)));
        }

        m_mappedMemory = allocationInfo.pMappedData;
    }

    VulkanBuffer::~VulkanBuffer()
    {
        destroy();
    }

    VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
        : m_allocator(other.m_allocator)
        , m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE))
        , m_allocation(std::exchange(other.m_allocation, nullptr))
        , m_sizeInBytes(std::exchange(other.m_sizeInBytes, 0))
        , m_mappedMemory(std::exchange(other.m_mappedMemory, nullptr))
    {
    }

    VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            m_allocator = other.m_allocator;
            m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
            m_allocation = std::exchange(other.m_allocation, nullptr);
            m_sizeInBytes = std::exchange(other.m_sizeInBytes, 0);
            m_mappedMemory = std::exchange(other.m_mappedMemory, nullptr);
        }
        return *this;
    }

    void VulkanBuffer::destroy()
    {
        // A moved-from buffer owns nothing.
        if (m_buffer != VK_NULL_HANDLE)
        {
            // Destroys the buffer and releases (and unmaps) its memory.
            vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
            m_buffer = VK_NULL_HANDLE;
            m_allocation = nullptr;
        }
    }

    vk::Buffer VulkanBuffer::handle() const
    {
        return vk::Buffer(m_buffer);
    }

    vk::DeviceSize VulkanBuffer::sizeInBytes() const
    {
        return m_sizeInBytes;
    }

    void VulkanBuffer::write(std::span<const std::byte> data, vk::DeviceSize offsetInBytes) const
    {
        MAGE_ASSERT(m_mappedMemory != nullptr, "Only CpuWritable buffers can be written to directly");
        MAGE_ASSERT(offsetInBytes + data.size() <= m_sizeInBytes,
                    "Writing {} bytes at offset {} overflows a buffer of {} bytes",
                    data.size(),
                    offsetInBytes,
                    m_sizeInBytes);

        std::memcpy(static_cast<std::byte*>(m_mappedMemory) + offsetInBytes, data.data(), data.size());

        // On some GPUs written data sits in a CPU cache until it is explicitly
        // flushed to the device. VMA makes this a no-op where it is not needed.
        vmaFlushAllocation(m_allocator, m_allocation, offsetInBytes, data.size());
    }
} // namespace mage::renderer
