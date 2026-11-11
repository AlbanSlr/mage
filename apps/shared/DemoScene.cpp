#include <mage/core/Math.hpp>
#include <mage/renderer/Primitives.hpp>
#include <mage/renderer/Renderer.hpp>

#include <shared/DemoScene.hpp>

#include <span>

namespace
{
    // Builds a model matrix. Read from right to left: the object is first
    // scaled, then rotated around the vertical axis, then moved into place.
    glm::mat4 makeModelMatrix(const glm::vec3& position, float yawInRadians, const glm::vec3& scale)
    {
        constexpr glm::vec3 UP_AXIS{0.0F, 1.0F, 0.0F};

        glm::mat4 modelMatrix{1.0F};
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, yawInRadians, UP_AXIS);
        modelMatrix = glm::scale(modelMatrix, scale);
        return modelMatrix;
    }
} // namespace

DemoScene::DemoScene(mage::renderer::Renderer& renderer)
    // One cube mesh is uploaded once and drawn several times with different
    // transforms and textures.
    : m_cubeMesh(renderer.createMesh(mage::renderer::makeCubeMesh()))
    , m_orangeTexture(renderer.createTexture(
          mage::renderer::makeCheckerboardTexture(8, 32, {235, 120, 40, 255}, {250, 235, 215, 255})))
    , m_blueTexture(renderer.createTexture(
          mage::renderer::makeCheckerboardTexture(8, 32, {40, 110, 220, 255}, {220, 235, 250, 255})))
    , m_groundTexture(renderer.createTexture(
          mage::renderer::makeCheckerboardTexture(16, 16, {70, 75, 80, 255}, {110, 115, 120, 255})))
{
    update(0.0F);
}

void DemoScene::update(float deltaTimeInSeconds)
{
    m_spinAngleInRadians += spinSpeedInRadiansPerSecond * deltaTimeInSeconds;

    m_drawCommands = {
        mage::renderer::DrawCommand{
            .mesh = m_cubeMesh,
            .texture = m_groundTexture,
            .modelMatrix = makeModelMatrix({0.0F, -0.55F, 0.0F}, 0.0F, {12.0F, 0.1F, 12.0F}),
        },
        mage::renderer::DrawCommand{
            .mesh = m_cubeMesh,
            .texture = m_orangeTexture,
            .modelMatrix = makeModelMatrix({0.0F, 0.0F, 0.0F}, m_spinAngleInRadians, glm::vec3{1.0F}),
        },
        mage::renderer::DrawCommand{
            .mesh = m_cubeMesh,
            .texture = m_blueTexture,
            .modelMatrix = makeModelMatrix({0.9F, 0.25F, -0.6F}, glm::radians(30.0F), glm::vec3{1.5F}),
        },
    };
}

std::span<const mage::renderer::DrawCommand> DemoScene::drawCommands() const
{
    return m_drawCommands;
}
