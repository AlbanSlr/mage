#include "vulkan/VulkanInstance.hpp"

#include "vulkan/VulkanApi.hpp"

#include <mage/core/Log.hpp>

#include <algorithm>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mage::renderer
{
    namespace
    {
        constexpr const char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

        // Called by the validation layer (and by the driver) every time they
        // have something to report. The signature is imposed by Vulkan.
        VKAPI_ATTR vk::Bool32 VKAPI_CALL
        debugMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                               vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
                               const vk::DebugUtilsMessengerCallbackDataEXT* callbackData,
                               void* /*userData*/)
        {
            const std::string_view message = callbackData->pMessage;

            if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            {
                log::error("[Vulkan] {}", message);
            }
            else if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            {
                log::warning("[Vulkan] {}", message);
            }
            else
            {
                log::debug("[Vulkan] {}", message);
            }

            // Returning true would make the Vulkan call that triggered the
            // message fail. That is only useful when testing the layers.
            return vk::False;
        }

        vk::DebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo()
        {
            return vk::DebugUtilsMessengerCreateInfoEXT{
                // Verbose and Info messages are left out: they describe normal
                // loader and driver activity and would flood the log.
                .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                                   | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral        // not tied to the spec
                               | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation   // specification violations
                               | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance, // valid but inefficient usage
                .pfnUserCallback = &debugMessengerCallback,
            };
        }

        bool isLayerAvailable(const vk::raii::Context& context, std::string_view layerName)
        {
            const std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

            return std::ranges::any_of(availableLayers,
                                       [layerName](const vk::LayerProperties& layer)
                                       { return layerName == std::string_view(layer.layerName.data()); });
        }

        bool isExtensionAvailable(const std::vector<vk::ExtensionProperties>& availableExtensions,
                                  std::string_view                            extensionName)
        {
            return std::ranges::any_of(availableExtensions,
                                       [extensionName](const vk::ExtensionProperties& extension)
                                       { return extensionName == std::string_view(extension.extensionName.data()); });
        }
    } // namespace

    VulkanInstance::VulkanInstance(const VulkanInstanceDescription& description)
    {
        // --- 1. Check the Vulkan version of the loader ------------------------
        // This is the highest version the installed Vulkan loader understands.
        // Each GPU additionally reports its own version (see VulkanDevice).
        const std::uint32_t loaderApiVersion = m_context.enumerateInstanceVersion();
        if (loaderApiVersion < vk::ApiVersion14)
        {
            throw std::runtime_error(
                std::format("MAGE requires Vulkan 1.4, but the installed Vulkan loader only supports {}.{}",
                            vk::apiVersionMajor(loaderApiVersion),
                            vk::apiVersionMinor(loaderApiVersion)));
        }

        // --- 2. Layers ----------------------------------------------------------
        // A layer is a library inserted between the application and the driver
        // that can inspect every call. The validation layer is the main one.
        std::vector<const char*> enabledLayers;
        if (description.enableValidation)
        {
            if (isLayerAvailable(m_context, VALIDATION_LAYER_NAME))
            {
                enabledLayers.push_back(VALIDATION_LAYER_NAME);
                m_validationEnabled = true;
            }
            else
            {
                log::warning("Vulkan validation was requested but {} is not installed (install the Vulkan SDK)",
                             VALIDATION_LAYER_NAME);
            }
        }

        // --- 3. Instance extensions ---------------------------------------------
        // Extensions add functionality that is not part of core Vulkan.
        // "Instance" extensions are not tied to a particular GPU: presenting to
        // a window, and the debug messenger, are the two MAGE needs.
        std::vector<const char*> enabledExtensions(description.requiredExtensions.begin(),
                                                   description.requiredExtensions.end());
        if (m_validationEnabled)
        {
            enabledExtensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        const std::vector<vk::ExtensionProperties> availableExtensions =
            m_context.enumerateInstanceExtensionProperties();
        for (const char* extensionName : enabledExtensions)
        {
            if (!isExtensionAvailable(availableExtensions, extensionName))
            {
                throw std::runtime_error(
                    std::format("Required Vulkan instance extension {} is not available", extensionName));
            }
        }

        // --- 4. Create the instance ---------------------------------------------
        // Purely informative, except for apiVersion: it is the highest Vulkan
        // version the application will use, and it enables that version's
        // core features.
        const vk::ApplicationInfo applicationInfo{
            .pApplicationName = description.applicationName.c_str(),
            .applicationVersion = vk::makeApiVersion(0, 0, 1, 0),
            .pEngineName = "MAGE",
            .engineVersion = vk::makeApiVersion(0, 0, 1, 0),
            .apiVersion = vk::ApiVersion14,
        };

        // The real debug messenger can only be created once the instance
        // exists, so it cannot report problems in vkCreateInstance itself.
        // Chaining its create info to `pNext` installs a temporary messenger
        // that is active just for the creation (and destruction) of the instance.
        const vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = makeDebugMessengerCreateInfo();

        const vk::InstanceCreateInfo instanceCreateInfo{
            .pNext = m_validationEnabled ? &debugMessengerCreateInfo : nullptr,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = static_cast<std::uint32_t>(enabledLayers.size()),
            .ppEnabledLayerNames = enabledLayers.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size()),
            .ppEnabledExtensionNames = enabledExtensions.data(),
        };

        m_instance = vk::raii::Instance(m_context, instanceCreateInfo);

        if (m_validationEnabled)
        {
            m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
        }

        log::info("Created Vulkan instance (loader {}.{}.{}, validation {})",
                  vk::apiVersionMajor(loaderApiVersion),
                  vk::apiVersionMinor(loaderApiVersion),
                  vk::apiVersionPatch(loaderApiVersion),
                  m_validationEnabled ? "enabled" : "disabled");
        for (const char* extensionName : enabledExtensions)
        {
            log::debug("  instance extension: {}", extensionName);
        }
    }

    const vk::raii::Instance& VulkanInstance::handle() const
    {
        return m_instance;
    }

    bool VulkanInstance::isValidationEnabled() const
    {
        return m_validationEnabled;
    }
} // namespace mage::renderer
