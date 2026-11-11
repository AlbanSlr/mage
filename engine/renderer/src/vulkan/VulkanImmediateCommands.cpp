#include "vulkan/VulkanImmediateCommands.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace mage::renderer
{
    VulkanImmediateCommands::VulkanImmediateCommands(const VulkanDevice& device)
        : m_device(device)
    {
        // A pool of its own, separate from the per-frame pools, so immediate
        // commands never interfere with a frame being recorded.
        m_commandPool = m_device.logicalDevice().createCommandPool(vk::CommandPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = m_device.graphicsQueueFamilyIndex(),
        });
    }

    void VulkanImmediateCommands::submit(const RecordFunction& recordCommands) const
    {
        const vk::raii::Device& logicalDevice = m_device.logicalDevice();

        // The command buffer is freed back to the pool at the end of this function.
        const std::vector<vk::raii::CommandBuffer> commandBuffers =
            logicalDevice.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
                .commandPool = *m_commandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            });
        const vk::raii::CommandBuffer& commandBuffer = commandBuffers.front();

        commandBuffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });
        recordCommands(commandBuffer);
        commandBuffer.end();

        const vk::CommandBufferSubmitInfo commandBufferInfo{
            .commandBuffer = *commandBuffer,
        };

        const vk::SubmitInfo2 submitInfo{
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
        };

        // The fence is how the CPU learns that the GPU finished the work.
        const vk::raii::Fence finishedFence = logicalDevice.createFence(vk::FenceCreateInfo{});
        m_device.graphicsQueue().submit2(submitInfo, *finishedFence);

        constexpr std::uint64_t NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
        static_cast<void>(logicalDevice.waitForFences(*finishedFence, vk::True, NO_TIMEOUT));
    }
} // namespace mage::renderer
