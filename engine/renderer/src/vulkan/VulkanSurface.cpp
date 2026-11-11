#include "vulkan/VulkanSurface.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanInstance.hpp"

#include <mage/platform/VulkanWindowSurface.hpp>
#include <mage/platform/Window.hpp>

namespace mage::renderer
{
    // The platform module creates the surface (the call differs per operating
    // system) and returns a raw C handle. Wrapping it in vk::raii::SurfaceKHR
    // transfers ownership: it is destroyed with vkDestroySurfaceKHR automatically.
    VulkanSurface::VulkanSurface(const VulkanInstance& instance, const platform::Window& window)
        : m_surface(instance.handle(), platform::createVulkanSurface(window, *instance.handle()))
    {
    }

    const vk::raii::SurfaceKHR& VulkanSurface::handle() const
    {
        return m_surface;
    }
} // namespace mage::renderer
