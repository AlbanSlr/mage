# mage_add_module(<name>
#     SOURCES               <files...>
#     [PUBLIC_DEPENDENCIES  <targets...>]   # needed by the module's public headers
#     [PRIVATE_DEPENDENCIES <targets...>]   # only needed by the module's .cpp files
#     [TEST_SOURCES         <files...>]     # Catch2 tests, built into mage_<name>_tests
# )
#
# Declares one engine module as the static library `mage_<name>` (alias
# `mage::<name>`). Every module follows the same folder layout:
#
#   engine/<name>/
#     include/mage/<name>/   public headers, included as <mage/<name>/File.hpp>
#     src/                   implementation files and private headers
#     tests/                 unit tests
#
# Module dependencies must only point "downward" (see docs/ARCHITECTURE.md);
# listing them explicitly here is what keeps that rule visible and enforced.
function(mage_add_module module_name)
    cmake_parse_arguments(PARSE_ARGV 1 MODULE
        ""
        ""
        "SOURCES;PUBLIC_DEPENDENCIES;PRIVATE_DEPENDENCIES;TEST_SOURCES"
    )

    set(target_name mage_${module_name})

    add_library(${target_name} STATIC ${MODULE_SOURCES})
    add_library(mage::${module_name} ALIAS ${target_name})

    target_include_directories(${target_name}
        PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_link_libraries(${target_name}
        PUBLIC  ${MODULE_PUBLIC_DEPENDENCIES}
        PRIVATE ${MODULE_PRIVATE_DEPENDENCIES} mage::project_options
    )

    mage_enable_clang_tidy(${target_name})

    if(MAGE_BUILD_TESTS AND MODULE_TEST_SOURCES)
        set(test_target_name ${target_name}_tests)

        add_executable(${test_target_name} ${MODULE_TEST_SOURCES})
        target_link_libraries(${test_target_name} PRIVATE
            ${target_name}
            mage::project_options
            Catch2::Catch2WithMain
        )

        # Registers every TEST_CASE as an individual CTest test.
        catch_discover_tests(${test_target_name})
    endif()
endfunction()

# mage_add_application(<name> SOURCES <files...> [DEPENDENCIES <targets...>])
#
# Declares an executable (sandbox, editor, tools) built on top of the engine.
function(mage_add_application application_name)
    cmake_parse_arguments(PARSE_ARGV 1 APPLICATION "" "" "SOURCES;DEPENDENCIES")

    add_executable(${application_name} ${APPLICATION_SOURCES})
    target_link_libraries(${application_name} PRIVATE
        ${APPLICATION_DEPENDENCIES}
        mage::project_options
    )

    mage_enable_clang_tidy(${application_name})
endfunction()
