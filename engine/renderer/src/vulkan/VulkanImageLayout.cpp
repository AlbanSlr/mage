#include "vulkan/VulkanImageLayout.hpp"

#include "vulkan/VulkanApi.hpp"

namespace mage::renderer
{
    void transitionImageLayout(const vk::raii::CommandBuffer& commandBuffer,
                               vk::Image                      image,
                               const ImageLayoutTransition&   transition)
    {
        const vk::ImageMemoryBarrier2 imageBarrier{
            .srcStageMask = transition.sourceStage,
            .srcAccessMask = transition.sourceAccess,
            .dstStageMask = transition.destinationStage,
            .dstAccessMask = transition.destinationAccess,
            .oldLayout = transition.oldLayout,
            .newLayout = transition.newLayout,

            // Barriers can also transfer an image between queue families.
            // "Ignored" on both sides means no transfer.
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,

            .image = image,
            .subresourceRange =
                vk::ImageSubresourceRange{
                    .aspectMask = transition.aspect,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        const vk::DependencyInfo dependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }
} // namespace mage::renderer
