mage_add_application(mage_sandbox
    SOURCES
        src/Main.cpp
    DEPENDENCIES
        mage::core
        mage::platform
        mage::renderer
        mage::apps_shared
)

# The sandbox loads compiled shaders at runtime: build them along with it.
add_dependencies(mage_sandbox mage_shaders)
