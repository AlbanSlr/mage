# Third-party dependencies, vendored as git submodules pinned to release tags.
#
# Each one is added with SYSTEM so its headers are treated as system headers:
# our warning flags and clang-tidy do not report issues inside code we do not own.
#
# Submodules that are checked out but not yet part of the build are wired in
# by the milestone that first needs them:
#   Vulkan-Headers, VulkanMemoryAllocator, SDL  -> Vulkan bring-up
#   glm                                         -> GPU resources / camera
#   imgui                                       -> editor shell
#   tracy                                       -> profiling

# --- spdlog (logging backend of mage::core) ---------------------------------
# Use std::format instead of the bundled fmt library, so MAGE has a single
# formatting syntax everywhere.
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
add_subdirectory(spdlog SYSTEM)

# --- Catch2 (unit tests) ----------------------------------------------------
if(MAGE_BUILD_TESTS)
    add_subdirectory(Catch2 SYSTEM)

    # Provides catch_discover_tests(), used by mage_add_module().
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Catch2/extras")
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
    include(Catch)
endif()
