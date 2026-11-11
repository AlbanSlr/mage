#include "vulkan/VulkanSwapchain.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanSurface.hpp"

#include <mage/core/Assert.hpp>
#include <mage/core/Log.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace mage::renderer
{
    namespace
    {
        // The surface format is the pixel format of the swapchain images plus
        // the colour space the display interprets them in.
        //
        // An sRGB format is preferred: shaders then work with linear colour
        // values and the GPU applies the sRGB encoding when writing the pixel,
        // which is what a standard monitor expects.
        vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        {
            for (const vk::SurfaceFormatKHR& availableFormat : availableFormats)
            {
                if (availableFormat.format == vk::Format::eB8G8R8A8Srgb
                    && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                {
                    return availableFormat;
                }
            }

            // Any format works; the first one is what the driver likes best.
            return availableFormats.front();
        }

        // The size of the swapchain images.
        vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D desiredExtent)
        {
            // Most window systems dictate the size: the swapchain must match
            // the window exactly. A width of UINT32_MAX is the special value
            // meaning "the application chooses" (Wayland does this).
            if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
            {
                return capabilities.currentExtent;
            }

            return vk::Extent2D{
                .width = std::clamp(
                    desiredExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                .height = std::clamp(
                    desiredExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
            };
        }

        std::uint32_t chooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
        {
            // One more than the minimum, so the application can always acquire
            // an image to render into without waiting for the driver to
            // release one.
            std::uint32_t imageCount = capabilities.minImageCount + 1;

            // maxImageCount == 0 means "no upper limit".
            if (capabilities.maxImageCount > 0)
            {
                imageCount = std::min(imageCount, capabilities.maxImageCount);
            }

            return imageCount;
        }
    } // namespace

    VulkanSwapchain::VulkanSwapchain(const VulkanDevice&  device,
                                     const VulkanSurface& surface,
                                     vk::Extent2D         desiredExtent)
        : m_device(device)
        , m_surface(surface)
    {
        create(desiredExtent, nullptr);
    }

    void VulkanSwapchain::recreate(vk::Extent2D desiredExtent)
    {
        // Keep the old swapchain alive while the new one is created from it,
        // then let it be destroyed at the end of this function.
        const vk::raii::SwapchainKHR oldSwapchain = std::move(m_swapchain);

        // Views and semaphores refer to the old images.
        m_imageViews.clear();
        m_renderFinishedSemaphores.clear();
        m_images.clear();

        create(desiredExtent, *oldSwapchain);
    }

    void VulkanSwapchain::create(vk::Extent2D desiredExtent, vk::SwapchainKHR oldSwapchain)
    {
        const vk::raii::PhysicalDevice& physicalDevice = m_device.physicalDevice();
        const vk::raii::Device&         logicalDevice = m_device.logicalDevice();
        const vk::SurfaceKHR            surface = *m_surface.handle();

        // --- 1. Ask what the surface supports ------------------------------------
        const vk::SurfaceCapabilitiesKHR        capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        const std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);

        MAGE_ASSERT(!surfaceFormats.empty(), "A surface always supports at least one format");

        m_surfaceFormat = chooseSurfaceFormat(surfaceFormats);
        m_extent = chooseExtent(capabilities, desiredExtent);

        // --- 2. Create the swapchain ---------------------------------------------
        const vk::SwapchainCreateInfoKHR swapchainCreateInfo{
            .surface = surface,
            .minImageCount = chooseImageCount(capabilities),
            .imageFormat = m_surfaceFormat.format,
            .imageColorSpace = m_surfaceFormat.colorSpace,
            .imageExtent = m_extent,

            // More than one layer is only used for stereoscopic (VR) rendering.
            .imageArrayLayers = 1,

            // What the images will be used for: here, rendered into directly.
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,

            // Exclusive: the images are only ever used by one queue family.
            .imageSharingMode = vk::SharingMode::eExclusive,

            // No extra rotation or flip (relevant on phones and tablets).
            .preTransform = capabilities.currentTransform,

            // The window is opaque: the alpha channel is not used to blend it
            // with the windows behind it.
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,

            // The present mode decides how rendered images reach the screen.
            // FIFO is classic vertical sync: images queue up and one is shown
            // per display refresh. No tearing, and it is the only mode every
            // Vulkan implementation is required to support.
            .presentMode = vk::PresentModeKHR::eFifo,

            // Pixels hidden by another window do not need to be rendered.
            .clipped = vk::True,

            .oldSwapchain = oldSwapchain,
        };

        m_swapchain = logicalDevice.createSwapchainKHR(swapchainCreateInfo);

        // --- 3. Retrieve the images and create one view per image ---------------
        // The driver may create more images than the requested minimum.
        m_images = m_swapchain.getImages();

        m_imageViews.reserve(m_images.size());
        m_renderFinishedSemaphores.reserve(m_images.size());

        for (const vk::Image swapchainImage : m_images)
        {
            const vk::ImageViewCreateInfo imageViewCreateInfo{
                .image = swapchainImage,
                .viewType = vk::ImageViewType::e2D,
                .format = m_surfaceFormat.format,

                // The whole image: its colour data, its single mip level and
                // its single array layer.
                .subresourceRange =
                    vk::ImageSubresourceRange{
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            m_imageViews.push_back(logicalDevice.createImageView(imageViewCreateInfo));
            m_renderFinishedSemaphores.push_back(logicalDevice.createSemaphore(vk::SemaphoreCreateInfo{}));
        }

        log::info("Created swapchain: {} images, {}x{}, {}",
                  m_images.size(),
                  m_extent.width,
                  m_extent.height,
                  vk::to_string(m_surfaceFormat.format));
    }

    VulkanSwapchain::AcquiredImage VulkanSwapchain::acquireNextImage(vk::Semaphore imageAvailableSemaphore) const
    {
        // The timeout (in nanoseconds) is how long to wait for an image to be
        // free; the maximum value means "wait as long as it takes".
        constexpr std::uint64_t NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();

        const vk::ResultValue<std::uint32_t> acquired =
            m_swapchain.acquireNextImage(NO_TIMEOUT, imageAvailableSemaphore, nullptr);

        return AcquiredImage{
            .result = acquired.result,
            .imageIndex = acquired.value,
        };
    }

    vk::SwapchainKHR VulkanSwapchain::handle() const
    {
        return *m_swapchain;
    }

    vk::Format VulkanSwapchain::imageFormat() const
    {
        return m_surfaceFormat.format;
    }

    vk::Extent2D VulkanSwapchain::extent() const
    {
        return m_extent;
    }

    vk::Image VulkanSwapchain::image(std::uint32_t imageIndex) const
    {
        MAGE_ASSERT(imageIndex < m_images.size(), "Swapchain image index {} is out of range", imageIndex);
        return m_images[imageIndex];
    }

    vk::ImageView VulkanSwapchain::imageView(std::uint32_t imageIndex) const
    {
        MAGE_ASSERT(imageIndex < m_imageViews.size(), "Swapchain image index {} is out of range", imageIndex);
        return *m_imageViews[imageIndex];
    }

    vk::Semaphore VulkanSwapchain::renderFinishedSemaphore(std::uint32_t imageIndex) const
    {
        MAGE_ASSERT(
            imageIndex < m_renderFinishedSemaphores.size(), "Swapchain image index {} is out of range", imageIndex);
        return *m_renderFinishedSemaphores[imageIndex];
    }
} // namespace mage::renderer
