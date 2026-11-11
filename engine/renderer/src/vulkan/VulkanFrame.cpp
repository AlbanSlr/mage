#include "vulkan/VulkanFrame.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"

#include <utility>
#include <vector>

namespace mage::renderer
{
    VulkanFrame::VulkanFrame(const VulkanDevice& device)
    {
        const vk::raii::Device& logicalDevice = device.logicalDevice();

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

        m_imageAvailableSemaphore = logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{});

        // Created already signalled: the first thing a frame does is wait for
        // this fence, and on the very first frame no GPU work was submitted
        // that would ever signal it.
        m_inFlightFence = logicalDevice.createFence(vk::FenceCreateInfo{
            .flags = vk::FenceCreateFlagBits::eSignaled,
        });
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
} // namespace mage::renderer
