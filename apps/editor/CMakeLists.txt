mage_add_application(mage_editor
    SOURCES
        src/ConsolePanel.cpp
        src/Editor.cpp
        src/Main.cpp
    DEPENDENCIES
        mage::core
        mage::platform
        mage::renderer
        mage::ui
        mage::apps_shared
)

# The editor loads compiled shaders at runtime: build them along with it.
add_dependencies(mage_editor mage_shaders)
