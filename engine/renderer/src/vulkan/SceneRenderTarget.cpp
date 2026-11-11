#include "vulkan/SceneRenderTarget.hpp"

#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanImage.hpp"

namespace mage::renderer
{
    SceneRenderTarget::SceneRenderTarget(const VulkanDevice&    device,
                                         const VulkanAllocator& allocator,
                                         vk::Extent2D           extent,
                                         vk::Format             depthFormat)
        : m_colorImage(device,
                       allocator,
                       VulkanImageDescription{
                           .extent = extent,
                           .format = COLOR_FORMAT,

                           // Rendered into, then sampled by the user interface.
                           .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                           .aspect = vk::ImageAspectFlagBits::eColor,
                           .alternateViewFormat = SAMPLED_COLOR_FORMAT,
                       })
        , m_depthImage(device,
                       allocator,
                       VulkanImageDescription{
                           .extent = extent,
                           .format = depthFormat,
                           .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           .aspect = vk::ImageAspectFlagBits::eDepth,
                       })
    {
    }

    vk::Extent2D SceneRenderTarget::extent() const
    {
        return m_colorImage.extent();
    }

    vk::Image SceneRenderTarget::colorImage() const
    {
        return m_colorImage.handle();
    }

    vk::ImageView SceneRenderTarget::colorRenderView() const
    {
        return m_colorImage.view();
    }

    vk::ImageView SceneRenderTarget::colorSampledView() const
    {
        return m_colorImage.alternateView();
    }

    vk::Image SceneRenderTarget::depthImage() const
    {
        return m_depthImage.handle();
    }

    vk::ImageView SceneRenderTarget::depthView() const
    {
        return m_depthImage.view();
    }
} // namespace mage::renderer
