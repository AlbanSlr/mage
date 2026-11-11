#pragma once

#include "vulkan/VulkanApi.hpp"

#include <cstdint>
#include <vector>

namespace mage::renderer
{
    class VulkanDevice;
    class VulkanSurface;

    // The images that are shown in the window.
    //
    // Vulkan has no "default framebuffer". To display something, the
    // application asks for a SWAPCHAIN: a small set of images owned by the
    // window system. Each frame, the application
    //   1. ACQUIRES one image that is not currently on screen,
    //   2. renders into it,
    //   3. PRESENTS it, which hands it back to the window system for display.
    //
    // The swapchain is tied to the size of the window. When the window is
    // resized it becomes "out of date" and must be recreated.
    class VulkanSwapchain
    {
    public:
        // `desiredExtent` is the framebuffer size of the window, in pixels.
        VulkanSwapchain(const VulkanDevice& device, const VulkanSurface& surface, vk::Extent2D desiredExtent);

        // Replaces the swapchain by a new one matching `desiredExtent`.
        // The caller must make sure the GPU no longer uses the current images
        // (see Renderer::recreateSwapchain).
        void recreate(vk::Extent2D desiredExtent);

        // Result of acquireNextImage().
        struct AcquiredImage
        {
            // eSuccess, eSuboptimalKHR (usable, but no longer matches the
            // surface exactly) or eErrorOutOfDateKHR (not usable: recreate).
            vk::Result result = vk::Result::eSuccess;

            // Index of the acquired image. Only meaningful when the result is
            // not eErrorOutOfDateKHR.
            std::uint32_t imageIndex = 0;
        };

        // Asks the window system for the next image to render into.
        // `imageAvailableSemaphore` is signalled once the image is really
        // available: acquisition only reserves the index, the image may still
        // be on screen for a short while.
        [[nodiscard]] AcquiredImage acquireNextImage(vk::Semaphore imageAvailableSemaphore) const;

        [[nodiscard]] vk::SwapchainKHR handle() const;
        [[nodiscard]] vk::Format imageFormat() const;
        [[nodiscard]] vk::Extent2D extent() const;

        [[nodiscard]] vk::Image image(std::uint32_t imageIndex) const;
        [[nodiscard]] vk::ImageView imageView(std::uint32_t imageIndex) const;

        // Semaphore to signal when rendering to the given image is finished;
        // presentation waits on it. There is one per image rather than one per
        // frame in flight because a semaphore handed to the presentation
        // engine stays in use until that same image is acquired again.
        [[nodiscard]] vk::Semaphore renderFinishedSemaphore(std::uint32_t imageIndex) const;

    private:
        // `oldSwapchain` (may be null) lets the driver reuse resources of the
        // swapchain being replaced and keep its current image on screen.
        void create(vk::Extent2D desiredExtent, vk::SwapchainKHR oldSwapchain);

        const VulkanDevice&  m_device;
        const VulkanSurface& m_surface;

        vk::raii::SwapchainKHR m_swapchain = nullptr;

        // The images belong to the swapchain: they are neither created nor
        // destroyed by the application, hence the non-owning vk::Image.
        std::vector<vk::Image> m_images;

        // An image cannot be used directly as a render target: Vulkan accesses
        // images through IMAGE VIEWS, which select which part of the image is
        // used (mip levels, array layers) and how its format is interpreted.
        std::vector<vk::raii::ImageView> m_imageViews;

        std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

        vk::SurfaceFormatKHR m_surfaceFormat;
        vk::Extent2D         m_extent;
    };
} // namespace mage::renderer
