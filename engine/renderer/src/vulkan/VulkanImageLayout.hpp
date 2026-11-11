#pragma once

#include "vulkan/VulkanApi.hpp"

namespace mage::renderer
{
    // Describes one change of IMAGE LAYOUT.
    //
    // A Vulkan image is always in a "layout" that states what it is currently
    // used for (being rendered to, being presented, being sampled by a
    // shader...), and the GPU may physically organise the pixel data
    // differently for each. The application must move the image to the right
    // layout before each use, with a pipeline barrier.
    //
    // A barrier also carries synchronisation information: which earlier
    // pipeline stages and memory accesses must be finished (source), and
    // which later ones must wait for the transition (destination).
    struct ImageLayoutTransition
    {
        vk::ImageLayout oldLayout = vk::ImageLayout::eUndefined;
        vk::ImageLayout newLayout = vk::ImageLayout::eUndefined;

        vk::PipelineStageFlags2 sourceStage;
        vk::AccessFlags2        sourceAccess;
        vk::PipelineStageFlags2 destinationStage;
        vk::AccessFlags2        destinationAccess;

        // Colour for colour images, depth for depth buffers.
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
    };

    // Records the barrier performing `transition` on the whole image.
    void transitionImageLayout(const vk::raii::CommandBuffer& commandBuffer,
                               vk::Image                      image,
                               const ImageLayoutTransition&   transition);
} // namespace mage::renderer
