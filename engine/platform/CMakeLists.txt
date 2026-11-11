mage_add_module(platform
    SOURCES
        src/FileSystem.cpp
        src/VulkanWindowSurface.cpp
        src/Window.cpp
    PUBLIC_DEPENDENCIES
        mage::core
        # Only for the Vulkan handle types in VulkanWindowSurface.hpp.
        Vulkan::Headers
    PRIVATE_DEPENDENCIES
        # SDL is an implementation detail: no SDL header is included from a
        # public MAGE header, so another windowing backend could replace it.
        SDL3::SDL3
)
