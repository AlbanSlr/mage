mage_add_module(core
    SOURCES
        src/Assert.cpp
        src/Log.cpp
    PRIVATE_DEPENDENCIES
        spdlog::spdlog
    TEST_SOURCES
        tests/AssertTests.cpp
        tests/LogTests.cpp
)

# Assertions are compiled in for Debug and RelWithDebInfo, and compiled out of
# Release. PUBLIC because MAGE_ASSERT is a macro expanded in every module.
target_compile_definitions(mage_core PUBLIC
    $<$<CONFIG:Debug,RelWithDebInfo>:MAGE_ENABLE_ASSERTS>
)
