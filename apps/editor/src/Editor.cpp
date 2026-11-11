#include "Editor.hpp"

#include "ConsolePanel.hpp"

#include <mage/core/Math.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Renderer.hpp>
#include <mage/ui/ImGui.hpp>

#include <shared/DemoScene.hpp>
#include <shared/FlyCameraController.hpp>

// Dear ImGui's "dock builder" functions, used to create the default panel
// layout from code, are not part of its stable public API yet and live in
// its internal header.
#include <imgui_internal.h>

#include <cstdint>

namespace
{
    // Panel names. Dear ImGui identifies windows by name: the same string is
    // used to draw a panel and to place it in the default layout.
    constexpr const char* VIEWPORT_PANEL_NAME = "Viewport";
    constexpr const char* SCENE_SETTINGS_PANEL_NAME = "Scene Settings";
    constexpr const char* STATISTICS_PANEL_NAME = "Statistics";
    constexpr const char* CONSOLE_PANEL_NAME = "Console";

    // Arranges the panels the way a new user first sees them:
    //
    //   +---------------------------+-----------+
    //   |                           |  Scene    |
    //   |         Viewport          |  Settings |
    //   |                           +-----------+
    //   +---------------------------+ Statistics|
    //   |          Console          |           |
    //   +---------------------------+-----------+
    void buildDefaultLayout(ImGuiID dockSpaceId)
    {
        // Start from an empty dock space covering the whole window.
        ImGui::DockBuilderRemoveNode(dockSpaceId);
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->Size);

        // Each split cuts a node in two and returns the new node on the given
        // side; the last parameter receives what remains on the other side.
        ImGuiID centerNodeId = dockSpaceId;
        ImGuiID rightNodeId = ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Right, 0.22F, nullptr, &centerNodeId);
        const ImGuiID bottomNodeId =
            ImGui::DockBuilderSplitNode(centerNodeId, ImGuiDir_Down, 0.28F, nullptr, &centerNodeId);
        const ImGuiID rightBottomNodeId =
            ImGui::DockBuilderSplitNode(rightNodeId, ImGuiDir_Down, 0.40F, nullptr, &rightNodeId);

        ImGui::DockBuilderDockWindow(VIEWPORT_PANEL_NAME, centerNodeId);
        ImGui::DockBuilderDockWindow(CONSOLE_PANEL_NAME, bottomNodeId);
        ImGui::DockBuilderDockWindow(SCENE_SETTINGS_PANEL_NAME, rightNodeId);
        ImGui::DockBuilderDockWindow(STATISTICS_PANEL_NAME, rightBottomNodeId);

        ImGui::DockBuilderFinish(dockSpaceId);
    }
} // namespace

Editor::Editor(mage::platform::Window& window, mage::renderer::Renderer& renderer, ConsolePanel& consolePanel)
    : m_window(window)
    , m_renderer(renderer)
    , m_consolePanel(consolePanel)
    , m_scene(renderer)
{
    resetCamera();
}

void Editor::update(float deltaTimeInSeconds)
{
    updateCamera(deltaTimeInSeconds);
    m_scene.update(deltaTimeInSeconds);

    drawDockSpace();
    drawMainMenuBar();

    if (m_showViewport)
    {
        drawViewportPanel();
    }
    else
    {
        m_isViewportHovered = false;
    }

    if (m_showSceneSettings)
    {
        drawSceneSettingsPanel();
    }
    if (m_showStatistics)
    {
        drawStatisticsPanel(deltaTimeInSeconds);
    }
    if (m_showConsole)
    {
        m_consolePanel.draw(m_showConsole);
    }
    if (m_showImGuiDemo)
    {
        // Dear ImGui's own demo window: a live catalogue of every widget.
        ImGui::ShowDemoWindow(&m_showImGuiDemo);
    }
}

mage::renderer::FrameDescription Editor::frameDescription() const
{
    return mage::renderer::FrameDescription{
        .camera = m_camera,
        .directionToLight = m_scene.directionToLight,
        .drawCommands = m_scene.drawCommands(),
    };
}

bool Editor::isExitRequested() const
{
    return m_exitRequested;
}

void Editor::drawDockSpace()
{
    const ImGuiID dockSpaceId = ImGui::GetID("MageEditorDockSpace");

    // The layout is saved to the settings file when the editor closes and
    // restored at the next start. The default layout is only built when
    // nothing was restored (first run) or when the user asks to reset it.
    const bool hasSavedLayout = ImGui::DockBuilderGetNode(dockSpaceId) != nullptr;
    if (!hasSavedLayout || m_resetLayoutRequested)
    {
        buildDefaultLayout(dockSpaceId);
        m_resetLayoutRequested = false;
    }

    // Creates a dock space filling the main window, below the menu bar.
    ImGui::DockSpaceOverViewport(dockSpaceId, ImGui::GetMainViewport());
}

void Editor::drawMainMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Exit"))
        {
            m_exitRequested = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        // A menu item given a bool pointer shows a check mark and toggles it.
        ImGui::MenuItem(VIEWPORT_PANEL_NAME, nullptr, &m_showViewport);
        ImGui::MenuItem(SCENE_SETTINGS_PANEL_NAME, nullptr, &m_showSceneSettings);
        ImGui::MenuItem(STATISTICS_PANEL_NAME, nullptr, &m_showStatistics);
        ImGui::MenuItem(CONSOLE_PANEL_NAME, nullptr, &m_showConsole);
        ImGui::Separator();
        ImGui::MenuItem("Dear ImGui Demo", nullptr, &m_showImGuiDemo);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout"))
        {
            m_showViewport = true;
            m_showSceneSettings = true;
            m_showStatistics = true;
            m_showConsole = true;
            m_resetLayoutRequested = true;
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void Editor::drawViewportPanel()
{
    m_isViewportHovered = false;

    // No padding: the scene image should touch the edges of the panel.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    const bool isPanelVisible = ImGui::Begin(
        VIEWPORT_PANEL_NAME, &m_showViewport, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (isPanelVisible)
    {
        // Space left inside the panel, in interface units.
        const ImVec2 panelSize = ImGui::GetContentRegionAvail();

        if (panelSize.x >= 1.0F && panelSize.y >= 1.0F)
        {
            // The scene is rendered at the panel's size in physical pixels,
            // which is larger than its size in interface units on a
            // high-DPI display. The new size takes effect next frame; until
            // then the current image is simply stretched to fit.
            const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
            m_renderer.setSceneViewportExtent(mage::platform::PixelExtent{
                .width = static_cast<std::uint32_t>(panelSize.x * framebufferScale.x),
                .height = static_cast<std::uint32_t>(panelSize.y * framebufferScale.y),
            });

            // The renderer exposes the offscreen scene image as a texture
            // identifier that Dear ImGui can draw like any other image.
            ImGui::Image(ImTextureRef(static_cast<ImTextureID>(m_renderer.sceneViewportTextureId())), panelSize);
            m_isViewportHovered = ImGui::IsItemHovered();
        }
    }
    ImGui::End();
}

void Editor::drawSceneSettingsPanel()
{
    if (ImGui::Begin(SCENE_SETTINGS_PANEL_NAME, &m_showSceneSettings))
    {
        // A negative width means "all the available width minus this much":
        // it reserves a fixed space on the right for the widget labels.
        ImGui::PushItemWidth(-ImGui::GetFontSize() * 11.0F);

        // Widgets edit the variable they are given in place and return true
        // on the frames where the user changed it.
        ImGui::SeparatorText("Light");
        ImGui::DragFloat3("Direction to light", &m_scene.directionToLight.x, 0.01F, -1.0F, 1.0F);

        // A zero-length direction cannot be normalized by the renderer.
        if (glm::dot(m_scene.directionToLight, m_scene.directionToLight) < 1.0e-4F)
        {
            m_scene.directionToLight = {0.0F, 1.0F, 0.0F};
        }

        ImGui::SeparatorText("Animation");
        ImGui::SliderFloat("Spin speed (rad/s)", &m_scene.spinSpeedInRadiansPerSecond, -4.0F, 4.0F);

        ImGui::SeparatorText("Camera");
        // SliderAngle edits a value stored in radians but displays degrees.
        ImGui::SliderAngle("Field of view", &m_camera.verticalFieldOfView, 20.0F, 110.0F);
        ImGui::DragFloat3("Position", &m_camera.position.x, 0.05F);
        if (ImGui::Button("Reset camera"))
        {
            resetCamera();
        }

        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::TextWrapped("Hold the right mouse button over the viewport to look around and move with "
                           "W/A/S/D, E/Q (physical key positions). Hold Shift to move faster.");
    }
    ImGui::End();
}

void Editor::drawStatisticsPanel(float deltaTimeInSeconds)
{
    if (ImGui::Begin(STATISTICS_PANEL_NAME, &m_showStatistics))
    {
        // Dear ImGui keeps a frame rate averaged over the last ~60 frames,
        // which is steadier to read than the duration of a single frame.
        const float averageFramesPerSecond = ImGui::GetIO().Framerate;

        ImGui::Text("Frame rate: %.1f FPS", static_cast<double>(averageFramesPerSecond));
        ImGui::Text("Frame time: %.2f ms (last frame %.2f ms)",
                    static_cast<double>(1000.0F / averageFramesPerSecond),
                    static_cast<double>(deltaTimeInSeconds * 1000.0F));

        ImGui::Separator();

        const mage::platform::PixelExtent viewportExtent = m_renderer.sceneViewportExtent();
        const mage::platform::PixelExtent windowExtent = m_window.framebufferExtent();

        ImGui::Text("Scene viewport: %u x %u", viewportExtent.width, viewportExtent.height);
        ImGui::Text("Window: %u x %u", windowExtent.width, windowExtent.height);
        ImGui::Text("Draw commands: %zu", m_scene.drawCommands().size());
    }
    ImGui::End();
}

void Editor::updateCamera(float deltaTimeInSeconds)
{
    // The camera only reacts to the mouse and keyboard while the user is
    // "flying" (right mouse button held, started over the viewport), so that
    // typing in a panel or clicking a widget never moves it.
    m_cameraController.update(m_window,
                              m_camera,
                              deltaTimeInSeconds,
                              FlyCameraPermissions{
                                  .canStartLooking = m_isViewportHovered,
                                  .canMove = m_cameraController.isLooking(),
                              });

    // While flying, the mouse is captured by the camera: hide it from the
    // interface so that panels do not react to it.
    ImGuiIO& io = ImGui::GetIO();
    if (m_cameraController.isLooking())
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    }
    else
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
}

void Editor::resetCamera()
{
    m_camera = mage::renderer::Camera{};
    m_camera.position = {0.0F, 1.5F, 5.0F};
    m_camera.pitch = glm::radians(-15.0F);
}
