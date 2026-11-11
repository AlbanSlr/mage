#pragma once

#include "vulkan/VulkanApi.hpp"

#include <span>
#include <string>

namespace mage::renderer
{
    struct VulkanInstanceDescription
    {
        std::string applicationName = "MAGE Application";

        // Instance extensions the caller needs, typically the ones required to
        // present to a window (see platform::requiredVulkanInstanceExtensions).
        std::span<const char* const> requiredExtensions;

        // Request the Khronos validation layer, which checks every Vulkan call
        // against the specification and reports misuse. It is only a request:
        // if the layer is not installed, a warning is logged and the instance
        // is created without it.
        bool enableValidation = false;
    };

    // The connection between the application and the Vulkan library.
    //
    // A VkInstance is the first Vulkan object every program creates. It holds
    // the per-application state: which API version the application targets, and
    // which global layers and extensions are enabled. Everything else (physical
    // devices, surfaces, the debug messenger) is obtained from it.
    class VulkanInstance
    {
    public:
        // Throws std::runtime_error if Vulkan 1.4 or a required extension is
        // not available, and vk::SystemError if instance creation fails.
        explicit VulkanInstance(const VulkanInstanceDescription& description);

        [[nodiscard]] const vk::raii::Instance& handle() const;

        [[nodiscard]] bool isValidationEnabled() const;

    private:
        // Members are destroyed in reverse declaration order: the debug
        // messenger first, then the instance, then the loader library.

        // Loads the Vulkan loader library (vulkan-1.dll / libvulkan.so.1) and
        // gives access to the few functions that work without an instance.
        vk::raii::Context m_context;

        vk::raii::Instance m_instance = nullptr;

        // Receives the messages of the validation layer and forwards them to
        // the MAGE log. Stays null when validation is disabled.
        vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

        bool m_validationEnabled = false;
    };
} // namespace mage::renderer
