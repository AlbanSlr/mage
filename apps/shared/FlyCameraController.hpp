#pragma once

#include <mage/core/Math.hpp>

namespace mage::platform
{
    class Window;
}

namespace mage::renderer
{
    struct Camera;
}

// What the controller is allowed to do this frame. An application that shares
// the mouse and keyboard with a user interface uses it to keep camera input
// from firing while the user interacts with a panel.
struct FlyCameraPermissions
{
    // Pressing the right mouse button may start looking around. Looking that
    // has already started always continues until the button is released.
    bool canStartLooking = true;

    // The movement keys move the camera.
    bool canMove = true;
};

// Moves a camera like a free-flying drone:
//
//   hold right mouse button + move the mouse   look around
//   W / S                                      forward / backward
//   A / D                                      left / right
//   E / Q                                      up / down
//   hold left shift                            move faster
//
// Keys are physical positions (see mage::platform::Key): on an AZERTY keyboard
// the same keys are labelled Z / S / Q / D and E / A.
class FlyCameraController
{
public:
    // Reads the input state of `window` and moves `camera` accordingly.
    // `deltaTimeInSeconds` is the duration of the previous frame, which makes
    // the movement speed independent of the frame rate.
    void update(mage::platform::Window& window,
                mage::renderer::Camera& camera,
                float                   deltaTimeInSeconds,
                FlyCameraPermissions    permissions = {});

    // True while the right mouse button is held and the mouse is captured.
    [[nodiscard]] bool isLooking() const;

private:
    static constexpr float MOVE_SPEED_IN_UNITS_PER_SECOND = 3.0F;
    static constexpr float FAST_MOVE_SPEED_MULTIPLIER = 4.0F;
    static constexpr float LOOK_SENSITIVITY_IN_RADIANS_PER_PIXEL = 0.0025F;

    // Looking exactly straight up or down would make the view direction
    // parallel to the "up" vector used to build the view matrix.
    static constexpr float MAX_PITCH_IN_RADIANS = glm::radians(89.0F);

    bool m_isLooking = false;
};
