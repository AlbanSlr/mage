#pragma once

#include "vulkan/VulkanApi.hpp"

#include <vk_mem_alloc.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace mage::renderer
{
    class VulkanAllocator;

    // Who can access the memory of a buffer.
    enum class BufferMemoryAccess : std::uint8_t
    {
        // The memory lives on the GPU and the CPU cannot touch it directly.
        // Fastest for the GPU to read; data gets there by copying from a
        // CpuWritable "staging" buffer. Used for vertex and index buffers.
        GpuOnly,

        // The CPU can write into the memory at any time through a pointer.
        // Used for staging buffers and for data that changes every frame
        // (uniform buffers).
        CpuWritable,
    };

    struct VulkanBufferDescription
    {
        vk::DeviceSize sizeInBytes = 0;

        // What the buffer will be used as (vertex buffer, uniform buffer,
        // source or destination of a copy...). Vulkan requires every use to be
        // declared at creation.
        vk::BufferUsageFlags usage;

        BufferMemoryAccess memoryAccess = BufferMemoryAccess::GpuOnly;
    };

    // A BUFFER: a block of GPU-accessible memory holding arbitrary bytes
    // (vertices, indices, shader constants...), together with the memory
    // allocation that backs it.
    class VulkanBuffer
    {
    public:
        VulkanBuffer(const VulkanAllocator& allocator, const VulkanBufferDescription& description);
        ~VulkanBuffer();

        // Owns a GPU resource: movable, not copyable.
        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;
        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        [[nodiscard]] vk::Buffer handle() const;
        [[nodiscard]] vk::DeviceSize sizeInBytes() const;

        // Copies `data` into the buffer, starting `offsetInBytes` from its
        // beginning. Only valid for CpuWritable buffers.
        void write(std::span<const std::byte> data, vk::DeviceSize offsetInBytes = 0) const;

    private:
        void destroy();

        VmaAllocator   m_allocator = nullptr;
        VkBuffer       m_buffer = VK_NULL_HANDLE;
        VmaAllocation  m_allocation = nullptr;
        vk::DeviceSize m_sizeInBytes = 0;

        // CPU address of the buffer memory. CpuWritable buffers stay mapped
        // for their whole life; null for GpuOnly buffers.
        void* m_mappedMemory = nullptr;
    };
} // namespace mage::renderer
