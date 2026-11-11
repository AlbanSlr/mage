mage_add_module(renderer
    SOURCES
        src/Renderer.cpp
        src/vulkan/TrianglePipeline.cpp
        src/vulkan/VulkanDevice.cpp
        src/vulkan/VulkanFrame.cpp
        src/vulkan/VulkanInstance.cpp
        src/vulkan/VulkanSurface.cpp
        src/vulkan/VulkanSwapchain.cpp
    PUBLIC_DEPENDENCIES
        mage::core
        mage::platform
    PRIVATE_DEPENDENCIES
        Vulkan::Headers
)

# Vulkan-Hpp configuration. See src/vulkan/VulkanApi.hpp for what each one does.
target_compile_definitions(mage_renderer PRIVATE
    VK_NO_PROTOTYPES
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

    # The Vulkan validation layers are requested in builds meant for development.
    $<$<CONFIG:Debug,RelWithDebInfo>:MAGE_ENABLE_VULKAN_VALIDATION>
)
