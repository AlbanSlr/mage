mage_add_module(ui
    SOURCES
        src/UserInterface.cpp
    PUBLIC_DEPENDENCIES
        mage::core
        mage::platform
        # Public on purpose: application code builds its panels by calling
        # Dear ImGui directly (through <mage/ui/ImGui.hpp>).
        imgui::imgui
    PRIVATE_DEPENDENCIES
        SDL3::SDL3
)
