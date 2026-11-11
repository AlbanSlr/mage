#pragma once

#include "vulkan/VulkanApi.hpp"

namespace mage::platform
{
    class Window;
}

namespace mage::renderer
{
    class VulkanInstance;

    // The Vulkan view of a window: "something images can be presented to".
    //
    // A VkSurfaceKHR is needed twice during start-up:
    //   - to pick a GPU, because not every GPU (or queue) can present to every
    //     surface, and
    //   - to create the swapchain, the set of images that are shown in the window.
    //
    // It must be destroyed before the instance and before the window, which
    // the member order in Renderer guarantees.
    class VulkanSurface
    {
    public:
        VulkanSurface(const VulkanInstance& instance, const platform::Window& window);

        [[nodiscard]] const vk::raii::SurfaceKHR& handle() const;

    private:
        vk::raii::SurfaceKHR m_surface;
    };
} // namespace mage::renderer
