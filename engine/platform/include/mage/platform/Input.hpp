#pragma once

#include <cstdint>

// Keyboard and mouse identifiers, independent of the windowing backend.
// The current state of each is queried on platform::Window.
namespace mage::platform
{
    // Keys are identified by their PHYSICAL position on the keyboard, named
    // after the US QWERTY layout. `Key::W` is the key in the top-left of the
    // letter block whatever the user's layout is (it is labelled Z on a French
    // AZERTY keyboard), which is what movement controls want.
    enum class Key : std::uint8_t
    {
        W,
        A,
        S,
        D,
        Q,
        E,
        Space,
        LeftShift,
        LeftControl,
        Escape,
    };

    enum class MouseButton : std::uint8_t
    {
        Left,
        Middle,
        Right,
    };

    // Mouse movement accumulated during one Window::pollEvents() call, in
    // pixels. Positive x is to the right, positive y is DOWN.
    struct MouseMotion
    {
        float deltaX = 0.0F;
        float deltaY = 0.0F;
    };
} // namespace mage::platform
