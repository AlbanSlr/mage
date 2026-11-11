cmake_minimum_required(VERSION 3.29)

project(MAGE
    VERSION 0.1.0
    DESCRIPTION "Microburst Advanced Game Engine"
    LANGUAGES C CXX
)

# MAGE is built with clang on every platform so that warnings, language
# extensions and generated code are identical on Windows and Linux.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "MAGE must be built with clang (found ${CMAKE_CXX_COMPILER_ID}). "
        "Configure with one of the presets in CMakePresets.json.")
endif()

# --- Options ---------------------------------------------------------------
option(MAGE_BUILD_TESTS         "Build the unit tests of each engine module"   ON)
option(MAGE_WARNINGS_AS_ERRORS  "Treat compiler warnings in MAGE code as errors" OFF)
option(MAGE_ENABLE_CLANG_TIDY   "Run clang-tidy on MAGE code while compiling"  OFF)

# --- Global language settings (also apply to third-party code) ---------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MAGE uses headers, not C++20 modules: skip the per-file module dependency
# scan that CMake otherwise runs for every C++20 source.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Put every executable in one folder per configuration: build/<preset>/bin/<Config>.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(MageProjectOptions)
include(MageModule)

if(MAGE_BUILD_TESTS)
    enable_testing()
endif()

# clangd (editor code completion and diagnostics) looks for compile_commands.json
# in the source root, so copy the generated one there after each build.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(mage_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${PROJECT_SOURCE_DIR}/compile_commands.json"
        COMMENT "Copying compile_commands.json to the source root for clangd"
        VERBATIM
    )
endif()

# Order matters: dependencies first, then the engine modules, then the apps.
add_subdirectory(third_party)
add_subdirectory(engine)
add_subdirectory(apps)
