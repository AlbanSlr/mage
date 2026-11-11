#pragma once

#include "vulkan/VulkanApi.hpp"

#include <cstdint>

namespace mage::renderer
{
    class VulkanInstance;
    class VulkanSurface;

    // The GPU the engine renders with. Vulkan splits "the GPU" in two objects:
    //
    //   - The PHYSICAL device (VkPhysicalDevice) is the hardware itself. It is
    //     not created, only enumerated and queried: name, limits, supported
    //     features, formats, queue families...
    //   - The LOGICAL device (VkDevice) is the application's own connection to
    //     that hardware, created with the specific features and extensions the
    //     application chose to enable. Almost every other Vulkan object
    //     (images, buffers, pipelines, ...) is created from it.
    //
    // Work is not sent to the device directly but to one of its QUEUES. Queues
    // are grouped in families; each family supports a set of operations
    // (graphics, compute, transfer, presentation to a given surface).
    class VulkanDevice
    {
    public:
        // Selects the most suitable GPU able to present to `surface` and
        // creates the logical device.
        //
        // Throws std::runtime_error if no installed GPU meets MAGE's requirements.
        VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface);

        [[nodiscard]] const vk::raii::PhysicalDevice& physicalDevice() const;
        [[nodiscard]] const vk::raii::Device& logicalDevice() const;

        // The best depth buffer format this GPU can render to.
        [[nodiscard]] vk::Format findDepthFormat() const;

        // The queue used for both rendering and presentation.
        [[nodiscard]] const vk::raii::Queue& graphicsQueue() const;
        [[nodiscard]] std::uint32_t graphicsQueueFamilyIndex() const;

    private:
        vk::raii::PhysicalDevice m_physicalDevice = nullptr;
        vk::raii::Device         m_logicalDevice = nullptr;

        // Queues are owned by the logical device; this is only a handle to one.
        vk::raii::Queue m_graphicsQueue = nullptr;
        std::uint32_t   m_graphicsQueueFamilyIndex = 0;
    };
} // namespace mage::renderer
