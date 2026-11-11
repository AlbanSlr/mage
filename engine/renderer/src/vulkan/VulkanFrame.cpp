#include "vulkan/VulkanFrame.hpp"

#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanDevice.hpp"

#include <utility>
#include <vector>

namespace mage::renderer
{
    VulkanFrame::VulkanFrame(const VulkanDevice&             device,
                             const VulkanAllocator&          allocator,
                             const vk::raii::DescriptorPool& descriptorPool,
                             vk::DescriptorSetLayout         frameDescriptorSetLayout,
                             vk::DeviceSize                  frameUniformsSizeInBytes)
        : m_frameUniformBuffer(allocator,
                               VulkanBufferDescription{
                                   .sizeInBytes = frameUniformsSizeInBytes,
                                   .usage = vk::BufferUsageFlagBits::eUniformBuffer,

                                   // Rewritten by the CPU every frame.
                                   .memoryAccess = BufferMemoryAccess::CpuWritable,
                               })
    {
        const vk::raii::Device& logicalDevice = device.logicalDevice();

        // --- Command pool and buffer --------------------------------------------------
        const vk::CommandPoolCreateInfo commandPoolCreateInfo{
            // Transient: buffers from this pool are re-recorded every frame,
            // which lets the driver pick a suitable allocation strategy.
            .flags = vk::CommandPoolCreateFlagBits::eTransient,

            // Command buffers can only be submitted to queues of the family
            // their pool was created for.
            .queueFamilyIndex = device.graphicsQueueFamilyIndex(),
        };
        m_commandPool = logicalDevice.createCommandPool(commandPoolCreateInfo);

        const vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
            .commandPool = *m_commandPool,

            // Primary buffers are submitted directly to a queue. Secondary
            // ones can only be called from a primary buffer.
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        std::vector<vk::raii::CommandBuffer> commandBuffers =
            logicalDevice.allocateCommandBuffers(commandBufferAllocateInfo);
        m_commandBuffer = std::move(commandBuffers.front());

        // --- Synchronisation objects --------------------------------------------------
        m_imageAvailableSemaphore = logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{});

        // Created already signalled: the first thing a frame does is wait for
        // this fence, and on the very first frame no GPU work was submitted
        // that would ever signal it.
        m_inFlightFence = logicalDevice.createFence(vk::FenceCreateInfo{
            .flags = vk::FenceCreateFlagBits::eSignaled,
        });

        // --- Per-frame descriptor set -------------------------------------------------
        std::vector<vk::raii::DescriptorSet> descriptorSets =
            logicalDevice.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
                .descriptorPool = *descriptorPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &frameDescriptorSetLayout,
            });
        m_frameDescriptorSet = std::move(descriptorSets.front());

        // Make binding 0 of the set point at the uniform buffer. This is done
        // once: afterwards only the CONTENT of the buffer changes each frame,
        // not which buffer the descriptor refers to.
        const vk::DescriptorBufferInfo bufferInfo{
            .buffer = m_frameUniformBuffer.handle(),
            .offset = 0,
            .range = frameUniformsSizeInBytes,
        };

        const vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *m_frameDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo,
        };

        logicalDevice.updateDescriptorSets(descriptorWrite, nullptr);
    }

    const vk::raii::CommandPool& VulkanFrame::commandPool() const
    {
        return m_commandPool;
    }

    const vk::raii::CommandBuffer& VulkanFrame::commandBuffer() const
    {
        return m_commandBuffer;
    }

    const vk::raii::Semaphore& VulkanFrame::imageAvailableSemaphore() const
    {
        return m_imageAvailableSemaphore;
    }

    const vk::raii::Fence& VulkanFrame::inFlightFence() const
    {
        return m_inFlightFence;
    }

    const VulkanBuffer& VulkanFrame::frameUniformBuffer() const
    {
        return m_frameUniformBuffer;
    }

    vk::DescriptorSet VulkanFrame::frameDescriptorSet() const
    {
        return *m_frameDescriptorSet;
    }
} // namespace mage::renderer
