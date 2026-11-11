#include <mage/platform/VulkanWindowSurface.hpp>
#include <mage/platform/Window.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace mage::platform
{
    // The window parameter is not read: it is there to guarantee, through the
    // type system, that a Vulkan-capable window (and therefore SDL's video
    // subsystem and Vulkan loader) exists before this is called.
    std::vector<const char*> requiredVulkanInstanceExtensions(const Window& /*window*/)
    {
        Uint32             extensionCount = 0;
        const char* const* extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (extensionNames == nullptr)
        {
            throw std::runtime_error(
                std::format("Failed to query the Vulkan instance extensions required by SDL: {}", SDL_GetError()));
        }

        return {extensionNames, extensionNames + extensionCount};
    }

    VkSurfaceKHR createVulkanSurface(const Window& window, VkInstance vulkanInstance)
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        // The third argument is an optional custom memory allocator for the
        // Vulkan driver; MAGE uses the driver's default allocator everywhere.
        if (!SDL_Vulkan_CreateSurface(window.nativeHandle(), vulkanInstance, nullptr, &surface))
        {
            throw std::runtime_error(std::format("Failed to create the Vulkan surface: {}", SDL_GetError()));
        }

        return surface;
    }
} // namespace mage::platform
