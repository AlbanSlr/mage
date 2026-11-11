#pragma once

#include <mage/core/Math.hpp>

namespace mage::renderer
{
    // A perspective camera: the point of view a frame is rendered from.
    //
    // The orientation is stored as two angles rather than a matrix, which is
    // the natural representation for a camera driven by mouse movement.
    struct Camera
    {
        glm::vec3 position{0.0F, 0.0F, 0.0F};

        // Rotation around the vertical (+Y) axis, in radians. At 0 the camera
        // looks along -Z; positive values turn it to the right.
        float yaw = 0.0F;

        // Rotation up (positive) or down (negative), in radians. Must stay
        // strictly between -pi/2 and +pi/2.
        float pitch = 0.0F;

        // Vertical angle covered by the view, in radians.
        float verticalFieldOfView = glm::radians(60.0F);

        // Only what lies between these two distances from the camera is
        // visible. Depth precision is spread over that range, so the near
        // plane should not be smaller than necessary.
        float nearPlaneDistance = 0.1F;
        float farPlaneDistance = 200.0F;

        // Unit vectors of the camera's own axes, in world space.
        [[nodiscard]] glm::vec3 forwardDirection() const;
        [[nodiscard]] glm::vec3 rightDirection() const;

        // World space -> view space (the world as seen from the camera).
        [[nodiscard]] glm::mat4 viewMatrix() const;

        // View space -> Vulkan clip space. `aspectRatio` is width / height of
        // the image being rendered.
        [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const;
    };
} // namespace mage::renderer
