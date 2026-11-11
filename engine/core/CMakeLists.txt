mage_add_module(core
    SOURCES
        src/Assert.cpp
        src/File.cpp
        src/Log.cpp
    PUBLIC_DEPENDENCIES
        glm::glm
    PRIVATE_DEPENDENCIES
        spdlog::spdlog
    TEST_SOURCES
        tests/AssertTests.cpp
        tests/FileTests.cpp
        tests/LogTests.cpp
)

# Assertions are compiled in for Debug and RelWithDebInfo, and compiled out of
# Release. PUBLIC because MAGE_ASSERT is a macro expanded in every module.
target_compile_definitions(mage_core PUBLIC
    $<$<CONFIG:Debug,RelWithDebInfo>:MAGE_ENABLE_ASSERTS>
)

# GLM configuration. PUBLIC so every module sees the exact same GLM: mixing
# configurations between files would silently produce incompatible matrices.
target_compile_definitions(mage_core PUBLIC
    # Projection matrices map depth to Vulkan's 0..1 range (OpenGL uses -1..1).
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    # Reject implicit conversions between GLM types (for example vec4 -> vec3).
    GLM_FORCE_EXPLICIT_CTOR
)
