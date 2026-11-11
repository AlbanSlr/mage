#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

// The platform-specific half of Vulkan presentation.
//
// Core Vulkan has no idea what a window is: showing an image on screen goes
// through the "window system integration" (WSI) extensions. A VkSurfaceKHR is
// the Vulkan object that represents "a window we can present to", and creating
// one needs a different extension and a different call on each platform
// (Win32, Wayland, X11, ...). SDL hides that difference, so these two functions
// are the only place where MAGE's windowing and Vulkan code meet.
//
// These functions use the plain C handle types (VkInstance, VkSurfaceKHR) so
// that the platform module does not depend on Vulkan-Hpp.
namespace mage::platform
{
    class Window;

    // Names of the instance extensions Vulkan needs to create a surface for
    // the given window on the current platform, for example
    // "VK_KHR_surface" + "VK_KHR_win32_surface" on Windows.
    //
    // The returned strings are owned by SDL and stay valid for the lifetime of
    // the program.
    [[nodiscard]] std::vector<const char*> requiredVulkanInstanceExtensions(const Window& window);

    // Creates the surface of the given window. The caller owns the returned
    // handle and must destroy it (vkDestroySurfaceKHR) before both the
    // instance and the window.
    //
    // Throws std::runtime_error on failure.
    [[nodiscard]] VkSurfaceKHR createVulkanSurface(const Window& window, VkInstance vulkanInstance);
} // namespace mage::platform
