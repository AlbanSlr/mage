mage_add_module(renderer
    SOURCES
        src/Camera.cpp
        src/Primitives.cpp
        src/Renderer.cpp
        src/RenderResources.cpp
        src/vulkan/MeshPipeline.cpp
        src/vulkan/VulkanAllocator.cpp
        src/vulkan/VulkanBuffer.cpp
        src/vulkan/VulkanDevice.cpp
        src/vulkan/VulkanFrame.cpp
        src/vulkan/VulkanImage.cpp
        src/vulkan/VulkanImageLayout.cpp
        src/vulkan/VulkanImmediateCommands.cpp
        src/vulkan/VulkanInstance.cpp
        src/vulkan/VulkanMemoryAllocatorImplementation.cpp
        src/vulkan/VulkanShaderModule.cpp
        src/vulkan/VulkanSurface.cpp
        src/vulkan/VulkanSwapchain.cpp
    PUBLIC_DEPENDENCIES
        mage::core
        mage::platform
    PRIVATE_DEPENDENCIES
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
    TEST_SOURCES
        tests/CameraTests.cpp
        tests/PrimitivesTests.cpp
)

# Vulkan-Hpp configuration. See src/vulkan/VulkanApi.hpp for what each one does.
target_compile_definitions(mage_renderer PRIVATE
    VK_NO_PROTOTYPES
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

    # Vulkan Memory Allocator: MAGE does not link against the Vulkan library,
    # so VMA must not reference the Vulkan functions directly (static) but
    # fetch them at runtime from the two entry points it is given (dynamic).
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1

    # The Vulkan validation layers are requested in builds meant for development.
    $<$<CONFIG:Debug,RelWithDebInfo>:MAGE_ENABLE_VULKAN_VALIDATION>
)

# The VMA implementation is third-party code compiled inside one of our source
# files: exclude that file from our warning set and from clang-tidy.
set_source_files_properties(src/vulkan/VulkanMemoryAllocatorImplementation.cpp PROPERTIES
    COMPILE_OPTIONS "-w"
    SKIP_LINTING ON
)
