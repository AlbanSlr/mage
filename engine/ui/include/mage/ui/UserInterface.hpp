#pragma once

#include <string>

namespace mage::platform
{
    class Window;
}

namespace mage::ui
{
    // Owns the Dear ImGui context and connects it to the window.
    //
    // Dear ImGui itself is independent of any platform or graphics API. It is
    // connected to the outside world by two "backends":
    //   - a PLATFORM backend feeds it input events, the window size and time.
    //     This class sets up the SDL3 one.
    //   - a RENDERER backend draws the triangles it generates. The Vulkan one
    //     is set up by renderer::Renderer when created with
    //     `enableUserInterface`.
    //
    // Lifetime: create the UserInterface after the window and BEFORE the
    // renderer, so that it is destroyed after the renderer.
    //
    // Each frame:
    //     window.pollEvents();
    //     renderer.beginUserInterfaceFrame();
    //     userInterface.beginFrame();
    //     ... ImGui:: calls ...
    //     userInterface.endFrame();
    //     renderer.drawFrame(...);
    class UserInterface
    {
    public:
        // The window must outlive the UserInterface.
        explicit UserInterface(platform::Window& window);
        ~UserInterface();

        UserInterface(const UserInterface&) = delete;
        UserInterface& operator=(const UserInterface&) = delete;
        UserInterface(UserInterface&&) = delete;
        UserInterface& operator=(UserInterface&&) = delete;

        // Starts a new interface frame. ImGui:: functions may be called from
        // here until endFrame().
        void beginFrame();

        // Finalises the frame: Dear ImGui turns everything declared since
        // beginFrame() into draw data, which the renderer then draws.
        void endFrame();

    private:
        platform::Window& m_window;

        // Dear ImGui saves window positions and docking layout to this file
        // between runs. It keeps a pointer to the string, which therefore
        // has to live as long as the context.
        std::string m_settingsFilePath;
    };
} // namespace mage::ui
