# Code used by more than one application (sandbox and editor) that is not, or
# not yet, generic enough to belong to an engine module.
#
# Headers are included as <shared/File.hpp>.
add_library(mage_apps_shared STATIC
    CommandLine.cpp
    DemoScene.cpp
    FlyCameraController.cpp
)
add_library(mage::apps_shared ALIAS mage_apps_shared)

target_include_directories(mage_apps_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(mage_apps_shared
    PUBLIC
        mage::core
        mage::platform
        mage::renderer
    PRIVATE
        mage::project_options
)

mage_enable_clang_tidy(mage_apps_shared)
