#include <mage/core/Math.hpp>
#include <mage/renderer/Camera.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    void checkVectorsAreClose(const glm::vec3& actual, const glm::vec3& expected)
    {
        constexpr float TOLERANCE = 1.0e-5F;

        CHECK(actual.x == Catch::Approx(expected.x).margin(TOLERANCE));
        CHECK(actual.y == Catch::Approx(expected.y).margin(TOLERANCE));
        CHECK(actual.z == Catch::Approx(expected.z).margin(TOLERANCE));
    }
} // namespace

TEST_CASE("A default camera looks along -Z with +X to its right", "[renderer][camera]")
{
    const mage::renderer::Camera camera;

    checkVectorsAreClose(camera.forwardDirection(), {0.0F, 0.0F, -1.0F});
    checkVectorsAreClose(camera.rightDirection(), {1.0F, 0.0F, 0.0F});
}

TEST_CASE("Positive yaw turns the camera to the right", "[renderer][camera]")
{
    mage::renderer::Camera camera;
    camera.yaw = glm::radians(90.0F);

    checkVectorsAreClose(camera.forwardDirection(), {1.0F, 0.0F, 0.0F});
    checkVectorsAreClose(camera.rightDirection(), {0.0F, 0.0F, 1.0F});
}

TEST_CASE("Positive pitch tilts the camera upward", "[renderer][camera]")
{
    mage::renderer::Camera camera;
    camera.pitch = glm::radians(45.0F);

    CHECK(camera.forwardDirection().y > 0.0F);
    CHECK(glm::length(camera.forwardDirection()) == Catch::Approx(1.0F));
}

TEST_CASE("The view matrix moves the camera position to the origin", "[renderer][camera]")
{
    mage::renderer::Camera camera;
    camera.position = {3.0F, 2.0F, 5.0F};

    const glm::vec4 cameraPositionInViewSpace = camera.viewMatrix() * glm::vec4(camera.position, 1.0F);

    checkVectorsAreClose(glm::vec3(cameraPositionInViewSpace), {0.0F, 0.0F, 0.0F});
}

TEST_CASE("The projection matrix follows Vulkan clip space conventions", "[renderer][camera]")
{
    const mage::renderer::Camera camera;
    const glm::mat4              viewProjection = camera.projectionMatrix(16.0F / 9.0F) * camera.viewMatrix();

    // Projects a world position and applies the perspective divide, giving
    // normalized device coordinates.
    const auto project = [&viewProjection](const glm::vec3& worldPosition)
    {
        const glm::vec4 clipPosition = viewProjection * glm::vec4(worldPosition, 1.0F);
        return glm::vec3(clipPosition) / clipPosition.w;
    };

    SECTION("a point above the view axis has a negative Y (Y points down)")
    {
        CHECK(project({0.0F, 1.0F, -5.0F}).y < 0.0F);
    }

    SECTION("a point to the right has a positive X")
    {
        CHECK(project({1.0F, 0.0F, -5.0F}).x > 0.0F);
    }

    SECTION("depth goes from 0 at the near plane to 1 at the far plane")
    {
        CHECK(project({0.0F, 0.0F, -camera.nearPlaneDistance}).z == Catch::Approx(0.0F).margin(1.0e-4F));
        CHECK(project({0.0F, 0.0F, -camera.farPlaneDistance}).z == Catch::Approx(1.0F).margin(1.0e-4F));
    }
}
