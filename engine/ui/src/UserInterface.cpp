#include <mage/core/Log.hpp>
#include <mage/platform/FileSystem.hpp>
#include <mage/platform/Window.hpp>
#include <mage/ui/UserInterface.hpp>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <stdexcept>

namespace mage::ui
{
    UserInterface::UserInterface(platform::Window& window)
        : m_window(window)
        , m_settingsFilePath((platform::executableDirectory() / "mage_user_interface.ini").string())
    {
        // --- Context ------------------------------------------------------------------
        // The context holds all of Dear ImGui's state. ImGui:: functions
        // operate on the current context, which this call also sets.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = m_settingsFilePath.c_str();

        // Docking lets windows be dragged into each other to form tabs and
        // split layouts, which is what an editor is built from.
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // --- Style --------------------------------------------------------------------
        ImGui::StyleColorsDark();

        // On a high-DPI display the interface would be tiny without scaling:
        // widget sizes and fonts are both multiplied by the display scale.
        const float displayScale = SDL_GetWindowDisplayScale(m_window.nativeHandle());
        if (displayScale > 0.0F)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(displayScale);
            style.FontScaleDpi = displayScale;
        }

        // --- Platform backend ---------------------------------------------------------
        if (!ImGui_ImplSDL3_InitForVulkan(m_window.nativeHandle()))
        {
            ImGui::DestroyContext();
            throw std::runtime_error("Failed to initialise the Dear ImGui SDL3 backend");
        }

        // Dear ImGui needs to see every input event to know what the mouse is
        // over, which keys are held, what text was typed...
        m_window.setNativeEventHandler([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });

        log::info("Initialised the user interface (Dear ImGui {}, display scale {:.2f})", IMGUI_VERSION, displayScale);
    }

    UserInterface::~UserInterface()
    {
        m_window.setNativeEventHandler(nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void UserInterface::beginFrame()
    {
        // The platform backend updates the display size, the time step and
        // the mouse cursor, then Dear ImGui starts collecting the new frame.
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void UserInterface::endFrame()
    {
        ImGui::Render();
    }
} // namespace mage::ui
