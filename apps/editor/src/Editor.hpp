#pragma once

#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Renderer.hpp>

#include <shared/DemoScene.hpp>
#include <shared/FlyCameraController.hpp>

namespace mage::platform
{
    class Window;
}

class ConsolePanel;

// The editor's user interface and state.
//
// The window is filled by a DOCK SPACE: an area into which panels (Dear ImGui
// windows) can be docked side by side or as tabs. Each panel is drawn by one
// private function of this class:
//
//   Viewport        the scene, rendered offscreen and displayed as an image
//   Scene Settings  tweakable values of the demo scene and camera
//   Statistics      frame timing and renderer numbers
//   Console         the engine log (ConsolePanel)
class Editor
{
public:
    Editor(mage::platform::Window& window, mage::renderer::Renderer& renderer, ConsolePanel& consolePanel);

    // Updates the scene and camera, and builds the interface for this frame.
    // Must be called between UserInterface::beginFrame() and endFrame().
    void update(float deltaTimeInSeconds);

    // What the renderer should draw this frame. Valid until the next update().
    [[nodiscard]] mage::renderer::FrameDescription frameDescription() const;

    // True once the user chose File > Exit.
    [[nodiscard]] bool isExitRequested() const;

private:
    void drawDockSpace();
    void drawMainMenuBar();
    void drawViewportPanel();
    void drawSceneSettingsPanel();
    void drawStatisticsPanel(float deltaTimeInSeconds);
    void updateCamera(float deltaTimeInSeconds);
    void resetCamera();

    mage::platform::Window&   m_window;
    mage::renderer::Renderer& m_renderer;
    ConsolePanel&             m_consolePanel;

    DemoScene              m_scene;
    mage::renderer::Camera m_camera;
    FlyCameraController    m_cameraController;

    // Panel visibility, toggled from the View menu.
    bool m_showViewport = true;
    bool m_showSceneSettings = true;
    bool m_showStatistics = true;
    bool m_showConsole = true;
    bool m_showImGuiDemo = false;

    // Set from the View menu; the default layout is rebuilt on the next frame.
    bool m_resetLayoutRequested = false;

    // True while the mouse is over the scene image, which is when the camera
    // may take over the mouse.
    bool m_isViewportHovered = false;

    bool m_exitRequested = false;
};
