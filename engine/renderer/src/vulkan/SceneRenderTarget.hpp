#pragma once

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanImage.hpp"

namespace mage::renderer
{
    class VulkanAllocator;
    class VulkanDevice;

    // Images the 3D scene is rendered into when it is NOT drawn straight to
    // the window: a colour image and its depth buffer.
    //
    // This is "offscreen rendering" (or render-to-texture). The editor uses
    // it to show the scene inside one of its panels: the scene is first
    // rendered into this colour image, which the user interface then displays
    // like any other texture.
    //
    // Colour encoding. The colour image is used through two views:
    //   - the scene is RENDERED through an sRGB view, so the GPU encodes the
    //     linear colours computed by the shaders, exactly as it would for an
    //     sRGB swapchain;
    //   - the user interface SAMPLES it through a UNORM view, which returns
    //     the stored bytes untouched. Those already-encoded values are then
    //     written as-is into the UNORM swapchain (see SwapchainColorEncoding).
    // Sampling through the sRGB view instead would decode the colours back to
    // linear, and the scene would come out too dark on screen.
    class SceneRenderTarget
    {
    public:
        static constexpr vk::Format COLOR_FORMAT = vk::Format::eR8G8B8A8Srgb;
        static constexpr vk::Format SAMPLED_COLOR_FORMAT = vk::Format::eR8G8B8A8Unorm;

        SceneRenderTarget(const VulkanDevice&    device,
                          const VulkanAllocator& allocator,
                          vk::Extent2D           extent,
                          vk::Format             depthFormat);

        [[nodiscard]] vk::Extent2D extent() const;

        [[nodiscard]] vk::Image colorImage() const;
        [[nodiscard]] vk::ImageView colorRenderView() const;  // sRGB: to render into
        [[nodiscard]] vk::ImageView colorSampledView() const; // UNORM: to display

        [[nodiscard]] vk::Image depthImage() const;
        [[nodiscard]] vk::ImageView depthView() const;

    private:
        VulkanImage m_colorImage;
        VulkanImage m_depthImage;
    };
} // namespace mage::renderer
