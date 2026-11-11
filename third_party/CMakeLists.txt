# Third-party dependencies, vendored as git submodules pinned to release tags.
#
# Each one is added with SYSTEM so its headers are treated as system headers:
# our warning flags and clang-tidy do not report issues inside code we do not own.
#
# Submodules that are checked out but not yet part of the build are wired in
# by the milestone that first needs them:
#   tracy  -> profiling

# --- spdlog (logging backend of mage::core) ---------------------------------
# Use std::format instead of the bundled fmt library, so MAGE has a single
# formatting syntax everywhere.
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
add_subdirectory(spdlog SYSTEM)

# --- Vulkan-Headers (Vulkan C API + Vulkan-Hpp C++ bindings) ----------------
# Header-only target Vulkan::Headers. MAGE does not link against a Vulkan
# library at build time: the Vulkan loader (vulkan-1.dll / libvulkan.so.1,
# installed with the GPU driver) is opened at runtime by vk::raii::Context.
add_subdirectory(Vulkan-Headers SYSTEM)

# --- Vulkan Memory Allocator (GPU memory allocation) -------------------------
# Header-only target GPUOpen::VulkanMemoryAllocator. Its implementation is
# compiled once, in engine/renderer/src/vulkan/VulkanMemoryAllocatorImplementation.cpp.
add_subdirectory(VulkanMemoryAllocator SYSTEM)

# --- GLM (vector and matrix math) ---------------------------------------------
# Used header-only; its configuration macros are set on mage::core.
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(glm SYSTEM)

# --- SDL3 (window, input, Vulkan surface creation) ---------------------------
# Built as a static library so the engine has no SDL3.dll to ship or locate.
set(SDL_SHARED       OFF CACHE BOOL "" FORCE)
set(SDL_STATIC       ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS        OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES     OFF CACHE BOOL "" FORCE)
add_subdirectory(SDL SYSTEM EXCLUDE_FROM_ALL)

# SDL's own sources produce warnings with recent clang versions. They are not
# ours to fix, so silence them to keep MAGE's build output readable.
target_compile_options(SDL3-static PRIVATE -w)

# --- Dear ImGui (editor and debug user interface) -----------------------------
# Dear ImGui ships as plain source files without a build script, so its
# library target is declared here: the core, plus the two "backends" MAGE uses
# to connect it to SDL3 (input, windows) and to Vulkan (drawing).
set(MAGE_IMGUI_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

add_library(imgui STATIC
    "${MAGE_IMGUI_DIRECTORY}/imgui.cpp"
    "${MAGE_IMGUI_DIRECTORY}/imgui_demo.cpp"
    "${MAGE_IMGUI_DIRECTORY}/imgui_draw.cpp"
    "${MAGE_IMGUI_DIRECTORY}/imgui_tables.cpp"
    "${MAGE_IMGUI_DIRECTORY}/imgui_widgets.cpp"
    "${MAGE_IMGUI_DIRECTORY}/backends/imgui_impl_sdl3.cpp"
    "${MAGE_IMGUI_DIRECTORY}/backends/imgui_impl_vulkan.cpp"
)
add_library(imgui::imgui ALIAS imgui)

target_include_directories(imgui SYSTEM PUBLIC
    "${MAGE_IMGUI_DIRECTORY}"
    "${MAGE_IMGUI_DIRECTORY}/backends"
)
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Headers)
target_compile_definitions(imgui PUBLIC
    # MAGE does not link against the Vulkan library: the Vulkan backend is
    # given a function to look up the Vulkan functions at runtime instead.
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES
    # Compile errors instead of silently using functions Dear ImGui has retired.
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)
target_compile_options(imgui PRIVATE -w)

# --- Catch2 (unit tests) ----------------------------------------------------
if(MAGE_BUILD_TESTS)
    add_subdirectory(Catch2 SYSTEM)

    # Provides catch_discover_tests(), used by mage_add_module().
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Catch2/extras")
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
    include(Catch)
endif()
