#include <mage/core/Log.hpp>
#include <mage/platform/Input.hpp>
#include <mage/platform/Window.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <utility>

namespace mage::platform
{
    namespace
    {
        // A scancode identifies a physical key position, independently of the
        // keyboard layout (see the comment on platform::Key).
        SDL_Scancode toSdlScancode(Key key)
        {
            switch (key)
            {
                case Key::W:
                    return SDL_SCANCODE_W;
                case Key::A:
                    return SDL_SCANCODE_A;
                case Key::S:
                    return SDL_SCANCODE_S;
                case Key::D:
                    return SDL_SCANCODE_D;
                case Key::Q:
                    return SDL_SCANCODE_Q;
                case Key::E:
                    return SDL_SCANCODE_E;
                case Key::Space:
                    return SDL_SCANCODE_SPACE;
                case Key::LeftShift:
                    return SDL_SCANCODE_LSHIFT;
                case Key::LeftControl:
                    return SDL_SCANCODE_LCTRL;
                case Key::Escape:
                    return SDL_SCANCODE_ESCAPE;
            }
            return SDL_SCANCODE_UNKNOWN;
        }

        SDL_MouseButtonFlags toSdlMouseButtonMask(MouseButton mouseButton)
        {
            switch (mouseButton)
            {
                case MouseButton::Left:
                    return SDL_BUTTON_LMASK;
                case MouseButton::Middle:
                    return SDL_BUTTON_MMASK;
                case MouseButton::Right:
                    return SDL_BUTTON_RMASK;
            }
            return 0;
        }
    } // namespace

    Window::Window(const WindowDescription& description)
    {
        // SDL subsystems are reference counted: each window initialises the
        // video subsystem and releases it again in its destructor.
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        {
            throw std::runtime_error(std::format("Failed to initialise the SDL video subsystem: {}", SDL_GetError()));
        }

        // SDL_WINDOW_VULKAN makes SDL load the Vulkan loader library and
        // prepares the window so a Vulkan surface can later be created for it.
        // SDL_WINDOW_HIGH_PIXEL_DENSITY asks for a full-resolution framebuffer
        // on high-DPI displays instead of an upscaled one.
        SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        if (description.resizable)
        {
            windowFlags |= SDL_WINDOW_RESIZABLE;
        }

        m_sdlWindow = SDL_CreateWindow(description.title.c_str(),
                                       static_cast<int>(description.width),
                                       static_cast<int>(description.height),
                                       windowFlags);
        if (m_sdlWindow == nullptr)
        {
            const std::string errorMessage = std::format("Failed to create the window: {}", SDL_GetError());
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            throw std::runtime_error(errorMessage);
        }

        const PixelExtent extent = framebufferExtent();
        log::info("Created window '{}' ({}x{} pixels)", description.title, extent.width, extent.height);
    }

    Window::~Window()
    {
        SDL_DestroyWindow(m_sdlWindow);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    void Window::pollEvents()
    {
        // Motion is accumulated over all the events of this call: several
        // mouse events can arrive between two frames.
        m_mouseMotion = MouseMotion{};

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (m_nativeEventHandler)
            {
                m_nativeEventHandler(event);
            }

            switch (event.type)
            {
                // SDL_EVENT_QUIT covers "the application was asked to quit"
                // (for example Alt+F4 on the last window).
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    m_closeRequested = true;
                    break;

                case SDL_EVENT_WINDOW_MINIMIZED:
                    m_minimized = true;
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    m_mouseMotion.deltaX += event.motion.xrel;
                    m_mouseMotion.deltaY += event.motion.yrel;
                    break;

                case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    m_minimized = false;
                    break;

                default:
                    break;
            }
        }
    }

    bool Window::isCloseRequested() const
    {
        return m_closeRequested;
    }

    bool Window::isMinimized() const
    {
        return m_minimized;
    }

    PixelExtent Window::framebufferExtent() const
    {
        int widthInPixels = 0;
        int heightInPixels = 0;
        if (!SDL_GetWindowSizeInPixels(m_sdlWindow, &widthInPixels, &heightInPixels))
        {
            throw std::runtime_error(std::format("Failed to query the window size: {}", SDL_GetError()));
        }

        return PixelExtent{
            .width = static_cast<std::uint32_t>(widthInPixels),
            .height = static_cast<std::uint32_t>(heightInPixels),
        };
    }

    bool Window::isKeyDown(Key key) const
    {
        // SDL keeps an array with the state of every key, indexed by scancode
        // and updated while events are processed.
        const bool* keyboardState = SDL_GetKeyboardState(nullptr);
        return keyboardState[toSdlScancode(key)];
    }

    bool Window::isMouseButtonDown(MouseButton mouseButton) const
    {
        const SDL_MouseButtonFlags pressedButtons = SDL_GetMouseState(nullptr, nullptr);
        return (pressedButtons & toSdlMouseButtonMask(mouseButton)) != 0;
    }

    MouseMotion Window::mouseMotion() const
    {
        return m_mouseMotion;
    }

    void Window::setMouseCaptured(bool captured)
    {
        if (!SDL_SetWindowRelativeMouseMode(m_sdlWindow, captured))
        {
            log::warning("Failed to change the mouse capture mode: {}", SDL_GetError());
        }
    }

    void Window::setNativeEventHandler(NativeEventHandler handler)
    {
        m_nativeEventHandler = std::move(handler);
    }

    SDL_Window* Window::nativeHandle() const
    {
        return m_sdlWindow;
    }
} // namespace mage::platform
