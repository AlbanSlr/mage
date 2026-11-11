#include <mage/core/Math.hpp>
#include <mage/platform/Input.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Camera.hpp>

#include <shared/FlyCameraController.hpp>

#include <algorithm>

void FlyCameraController::update(mage::platform::Window& window,
                                 mage::renderer::Camera& camera,
                                 float                   deltaTimeInSeconds,
                                 FlyCameraPermissions    permissions)
{
    using mage::platform::Key;

    // --- Look ------------------------------------------------------------------------
    // The mouse is captured only while the right button is held, so the cursor
    // stays usable the rest of the time.
    const bool isRightButtonDown = window.isMouseButtonDown(mage::platform::MouseButton::Right);
    const bool wantsToLook = isRightButtonDown && (m_isLooking || permissions.canStartLooking);
    if (wantsToLook != m_isLooking)
    {
        window.setMouseCaptured(wantsToLook);
        m_isLooking = wantsToLook;
    }

    if (m_isLooking)
    {
        const mage::platform::MouseMotion mouseMotion = window.mouseMotion();

        camera.yaw += mouseMotion.deltaX * LOOK_SENSITIVITY_IN_RADIANS_PER_PIXEL;

        // Mouse Y grows downward, but moving the mouse up should look up.
        camera.pitch -= mouseMotion.deltaY * LOOK_SENSITIVITY_IN_RADIANS_PER_PIXEL;
        camera.pitch = std::clamp(camera.pitch, -MAX_PITCH_IN_RADIANS, MAX_PITCH_IN_RADIANS);
    }

    // --- Move ------------------------------------------------------------------------
    if (!permissions.canMove)
    {
        return;
    }

    glm::vec3 moveDirection{0.0F};

    if (window.isKeyDown(Key::W))
    {
        moveDirection += camera.forwardDirection();
    }
    if (window.isKeyDown(Key::S))
    {
        moveDirection -= camera.forwardDirection();
    }
    if (window.isKeyDown(Key::D))
    {
        moveDirection += camera.rightDirection();
    }
    if (window.isKeyDown(Key::A))
    {
        moveDirection -= camera.rightDirection();
    }
    if (window.isKeyDown(Key::E))
    {
        moveDirection.y += 1.0F;
    }
    if (window.isKeyDown(Key::Q))
    {
        moveDirection.y -= 1.0F;
    }

    // Normalizing keeps diagonal movement from being faster than straight
    // movement. A zero vector (no key pressed) cannot be normalized.
    if (glm::dot(moveDirection, moveDirection) > 0.0F)
    {
        float moveSpeed = MOVE_SPEED_IN_UNITS_PER_SECOND;
        if (window.isKeyDown(Key::LeftShift))
        {
            moveSpeed *= FAST_MOVE_SPEED_MULTIPLIER;
        }

        camera.position += glm::normalize(moveDirection) * moveSpeed * deltaTimeInSeconds;
    }
}

bool FlyCameraController::isLooking() const
{
    return m_isLooking;
}
