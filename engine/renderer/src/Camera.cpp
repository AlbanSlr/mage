#include <mage/core/Math.hpp>
#include <mage/renderer/Camera.hpp>

#include <cmath>

namespace mage::renderer
{
    namespace
    {
        constexpr glm::vec3 WORLD_UP_DIRECTION{0.0F, 1.0F, 0.0F};
    } // namespace

    glm::vec3 Camera::forwardDirection() const
    {
        // Spherical coordinates: yaw turns around the vertical axis, pitch
        // tilts up or down. With both at 0 the result is (0, 0, -1).
        return glm::vec3{
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            -std::cos(yaw) * std::cos(pitch),
        };
    }

    glm::vec3 Camera::rightDirection() const
    {
        // The right direction ignores pitch: it always stays horizontal.
        return glm::vec3{std::cos(yaw), 0.0F, std::sin(yaw)};
    }

    glm::mat4 Camera::viewMatrix() const
    {
        return glm::lookAt(position, position + forwardDirection(), WORLD_UP_DIRECTION);
    }

    glm::mat4 Camera::projectionMatrix(float aspectRatio) const
    {
        glm::mat4 projection = glm::perspective(verticalFieldOfView, aspectRatio, nearPlaneDistance, farPlaneDistance);

        // GLM follows the OpenGL convention where +Y points up in clip space.
        // In Vulkan +Y points DOWN, so the Y axis is flipped here. Without
        // this the image would be rendered upside down.
        projection[1][1] *= -1.0F;

        return projection;
    }
} // namespace mage::renderer
