# mage::project_options
#
# An INTERFACE target carrying the compiler flags shared by all MAGE code
# (engine modules, apps and tests). Third-party code never links against it,
# so our strict warning set does not apply to code we do not own.

add_library(mage_project_options INTERFACE)
add_library(mage::project_options ALIAS mage_project_options)

target_compile_options(mage_project_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow                # a local variable hides another variable
    -Wconversion            # implicit conversions that may change a value
    -Wsign-conversion       # implicit signed <-> unsigned conversions
    -Wold-style-cast        # C-style casts; use static_cast and friends
    -Wnon-virtual-dtor      # polymorphic class without a virtual destructor
    -Woverloaded-virtual    # a derived function hides a virtual one
    -Wimplicit-fallthrough  # switch case falling through without [[fallthrough]]
    $<$<BOOL:${MAGE_WARNINGS_AS_ERRORS}>:-Werror>
)

if(WIN32)
    target_compile_definitions(mage_project_options INTERFACE
        NOMINMAX             # stop <windows.h> from defining min()/max() macros
        WIN32_LEAN_AND_MEAN  # exclude rarely used parts of <windows.h>
    )
endif()

# mage_enable_clang_tidy(<target>)
#
# Runs clang-tidy alongside the compiler for the given target when the
# MAGE_ENABLE_CLANG_TIDY option is ON. Rules live in the root .clang-tidy file.
function(mage_enable_clang_tidy target_name)
    if(NOT MAGE_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(MAGE_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set_target_properties(${target_name} PROPERTIES
        CXX_CLANG_TIDY "${MAGE_CLANG_TIDY_EXECUTABLE}"
    )
endfunction()
