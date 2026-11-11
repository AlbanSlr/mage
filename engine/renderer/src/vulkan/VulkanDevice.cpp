#include "vulkan/VulkanDevice.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSurface.hpp"

#include <mage/core/Log.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mage::renderer
{
    namespace
    {
        // Device extensions add GPU-specific functionality. The swapchain is
        // one: a GPU without a display output (a compute card) has no use for it.
        constexpr std::array<const char*, 1> REQUIRED_DEVICE_EXTENSIONS = {
            vk::KHRSwapchainExtensionName,
        };

        // The result of inspecting one GPU.
        struct PhysicalDeviceCandidate
        {
            std::uint32_t graphicsQueueFamilyIndex = 0;

            // Higher is better; used to choose between several suitable GPUs.
            int score = 0;
        };

        // Finds a queue family that can both execute graphics commands and
        // present images to the surface.
        //
        // Vulkan allows these to be two different families, but every desktop
        // GPU exposes one family that does both. Requiring a single family
        // keeps the swapchain and submission code simpler (no sharing of images
        // between queue families).
        std::optional<std::uint32_t> findGraphicsAndPresentQueueFamily(const vk::raii::PhysicalDevice& physicalDevice,
                                                                       vk::SurfaceKHR                  surface)
        {
            const std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

            for (std::uint32_t familyIndex = 0; familyIndex < queueFamilies.size(); ++familyIndex)
            {
                const bool supportsGraphics =
                    static_cast<bool>(queueFamilies[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics);
                const bool supportsPresent = physicalDevice.getSurfaceSupportKHR(familyIndex, surface) == vk::True;

                if (supportsGraphics && supportsPresent)
                {
                    return familyIndex;
                }
            }

            return std::nullopt;
        }

        bool supportsRequiredExtensions(const vk::raii::PhysicalDevice& physicalDevice)
        {
            const std::vector<vk::ExtensionProperties> availableExtensions =
                physicalDevice.enumerateDeviceExtensionProperties();

            return std::ranges::all_of(
                REQUIRED_DEVICE_EXTENSIONS,
                [&availableExtensions](std::string_view requiredExtensionName)
                {
                    return std::ranges::any_of(
                        availableExtensions,
                        [requiredExtensionName](const vk::ExtensionProperties& extension)
                        { return requiredExtensionName == std::string_view(extension.extensionName.data()); });
                });
        }

        // Returns nothing if the GPU cannot be used by MAGE, and logs why.
        std::optional<PhysicalDeviceCandidate> evaluatePhysicalDevice(const vk::raii::PhysicalDevice& physicalDevice,
                                                                      vk::SurfaceKHR                  surface)
        {
            const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
            const std::string_view             deviceName = properties.deviceName.data();

            // Vulkan 1.4 makes everything MAGE relies on (dynamic rendering,
            // synchronization2, ...) a mandatory core feature, so checking the
            // version replaces checking each feature individually.
            if (properties.apiVersion < vk::ApiVersion14)
            {
                log::info("  {}: rejected, supports Vulkan {}.{} but 1.4 is required",
                          deviceName,
                          vk::apiVersionMajor(properties.apiVersion),
                          vk::apiVersionMinor(properties.apiVersion));
                return std::nullopt;
            }

            if (!supportsRequiredExtensions(physicalDevice))
            {
                log::info("  {}: rejected, a required device extension is missing", deviceName);
                return std::nullopt;
            }

            const std::optional<std::uint32_t> queueFamilyIndex =
                findGraphicsAndPresentQueueFamily(physicalDevice, surface);
            if (!queueFamilyIndex.has_value())
            {
                log::info("  {}: rejected, no queue family supports both graphics and presentation", deviceName);
                return std::nullopt;
            }

            // Prefer a dedicated graphics card to the GPU integrated in the CPU.
            int score = 0;
            switch (properties.deviceType)
            {
                case vk::PhysicalDeviceType::eDiscreteGpu:
                    score = 1000;
                    break;
                case vk::PhysicalDeviceType::eIntegratedGpu:
                    score = 100;
                    break;
                default:
                    score = 1; // Virtual GPUs and software (CPU) implementations.
                    break;
            }

            log::info("  {}: suitable ({}, score {})", deviceName, vk::to_string(properties.deviceType), score);

            return PhysicalDeviceCandidate{
                .graphicsQueueFamilyIndex = *queueFamilyIndex,
                .score = score,
            };
        }
    } // namespace

    VulkanDevice::VulkanDevice(const VulkanInstance& instance, const VulkanSurface& surface)
    {
        // --- 1. Select the physical device --------------------------------------
        const std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.handle().enumeratePhysicalDevices();

        log::info("Selecting a GPU among {} physical device(s):", physicalDevices.size());

        int bestScore = 0;
        for (const vk::raii::PhysicalDevice& physicalDevice : physicalDevices)
        {
            const std::optional<PhysicalDeviceCandidate> candidate =
                evaluatePhysicalDevice(physicalDevice, *surface.handle());

            if (candidate.has_value() && candidate->score > bestScore)
            {
                bestScore = candidate->score;
                m_graphicsQueueFamilyIndex = candidate->graphicsQueueFamilyIndex;
                m_physicalDevice = physicalDevice;
            }
        }

        if (bestScore == 0)
        {
            throw std::runtime_error("No GPU supporting Vulkan 1.4 and presentation to the window was found");
        }

        // --- 2. Describe the queues to create -----------------------------------
        // Queues are created together with the logical device. One queue from
        // the selected family is enough. Its priority (0..1) only matters
        // relative to other queues of the same device.
        const float queuePriority = 1.0F;

        const vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = m_graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        // --- 3. Choose the features to enable -----------------------------------
        // Supporting a feature is not enough: Vulkan features are opt-in and
        // must be enabled explicitly when the logical device is created.
        // Features are grouped in one structure per Vulkan version, and
        // structures are linked together through their `pNext` pointer.
        //
        //   synchronization2: simpler, more consistent API to express how
        //     commands depend on each other (vkCmdPipelineBarrier2, vkQueueSubmit2).
        //   dynamicRendering: start rendering by directly naming the images to
        //     draw into, without the VkRenderPass and VkFramebuffer objects
        //     that older Vulkan required.
        //   shaderDrawParameters: lets shaders read the parameters of the draw
        //     call (first vertex, first instance, draw index). The Slang
        //     compiler relies on it to implement SV_VertexID.
        //
        // The resulting chain is: deviceCreateInfo -> vulkan13Features -> vulkan11Features.
        vk::PhysicalDeviceVulkan11Features vulkan11Features{
            .shaderDrawParameters = vk::True,
        };

        vk::PhysicalDeviceVulkan13Features vulkan13Features{
            .pNext = &vulkan11Features,
            .synchronization2 = vk::True,
            .dynamicRendering = vk::True,
        };

        // --- 4. Create the logical device ---------------------------------------
        const vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &vulkan13Features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<std::uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
            .ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data(),
        };

        m_logicalDevice = m_physicalDevice.createDevice(deviceCreateInfo);

        // Queue index 0: the first (and only) queue created in that family.
        m_graphicsQueue = m_logicalDevice.getQueue(m_graphicsQueueFamilyIndex, 0);

        const vk::PhysicalDeviceProperties properties = m_physicalDevice.getProperties();
        log::info("Using GPU '{}' (Vulkan {}.{}.{}, queue family {})",
                  std::string_view(properties.deviceName.data()),
                  vk::apiVersionMajor(properties.apiVersion),
                  vk::apiVersionMinor(properties.apiVersion),
                  vk::apiVersionPatch(properties.apiVersion),
                  m_graphicsQueueFamilyIndex);
    }

    const vk::raii::PhysicalDevice& VulkanDevice::physicalDevice() const
    {
        return m_physicalDevice;
    }

    const vk::raii::Device& VulkanDevice::logicalDevice() const
    {
        return m_logicalDevice;
    }

    const vk::raii::Queue& VulkanDevice::graphicsQueue() const
    {
        return m_graphicsQueue;
    }

    std::uint32_t VulkanDevice::graphicsQueueFamilyIndex() const
    {
        return m_graphicsQueueFamilyIndex;
    }
} // namespace mage::renderer
