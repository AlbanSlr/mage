#pragma once

#include <mage/platform/Input.hpp>

#include <cstdint>
#include <string>

// Forward declaration of SDL's window type, so this header does not need to
// include any SDL header.
struct SDL_Window;

namespace mage::platform
{
    // A size measured in physical pixels.
    struct PixelExtent
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        [[nodiscard]] bool operator==(const PixelExtent&) const = default;
    };

    struct WindowDescription
    {
        std::string   title = "MAGE";
        std::uint32_t width = 1280; // In screen coordinates; see Window::framebufferExtent().
        std::uint32_t height = 720;
        bool          resizable = true;
    };

    // An operating system window that Vulkan can render into.
    //
    // The window itself knows nothing about Vulkan beyond being created as
    // "Vulkan capable"; the link between the two (the surface) is made in
    // VulkanWindowSurface.hpp.
    class Window
    {
    public:
        // Throws std::runtime_error if the window cannot be created.
        explicit Window(const WindowDescription& description);
        ~Window();

        // A window is a unique OS resource: it cannot be copied, and it is not
        // moved either so that references to it stay valid for its lifetime.
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        // Processes the operating system events received since the last call
        // (close button, minimise, ...). Must be called once per frame, from
        // the thread that created the window.
        void pollEvents();

        // True once the user asked to close the window.
        [[nodiscard]] bool isCloseRequested() const;

        // A minimised window has a zero-sized framebuffer: nothing can be
        // rendered to it until it is restored.
        [[nodiscard]] bool isMinimized() const;

        // Size of the drawable area in pixels. This is the size the swapchain
        // images must have. On high-DPI displays it is larger than the size in
        // screen coordinates the window was created with.
        [[nodiscard]] PixelExtent framebufferExtent() const;

        // --- Input -----------------------------------------------------------------
        // Input is polled: these return the state as of the last pollEvents().

        [[nodiscard]] bool isKeyDown(Key key) const;
        [[nodiscard]] bool isMouseButtonDown(MouseButton mouseButton) const;

        // Mouse movement received during the last pollEvents() call.
        [[nodiscard]] MouseMotion mouseMotion() const;

        // While captured, the cursor is hidden and locked to the window, and
        // the mouse reports unbounded relative motion: what a first-person
        // camera needs to keep turning without the cursor hitting a screen edge.
        void setMouseCaptured(bool captured);

        // The underlying SDL window, for code that must talk to SDL directly.
        [[nodiscard]] SDL_Window* nativeHandle() const;

    private:
        SDL_Window* m_sdlWindow = nullptr;
        MouseMotion m_mouseMotion;
        bool        m_closeRequested = false;
        bool        m_minimized = false;
    };
} // namespace mage::platform
