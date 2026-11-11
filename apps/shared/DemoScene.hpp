#pragma once

#include <mage/core/Math.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Renderer.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <span>

// The small test scene shown by the sandbox and the editor until the engine
// has a real scene system: a ground slab, a spinning cube, and a second cube
// that intersects the first one to show the depth test at work.
class DemoScene
{
public:
    // Uploads the meshes and textures the scene uses.
    explicit DemoScene(mage::renderer::Renderer& renderer);

    // Advances the animation by the duration of the previous frame.
    void update(float deltaTimeInSeconds);

    // What to draw this frame. Valid until the next call to update().
    [[nodiscard]] std::span<const mage::renderer::DrawCommand> drawCommands() const;

    // Tweakable settings.
    glm::vec3 directionToLight{0.4F, 1.0F, 0.6F};
    float     spinSpeedInRadiansPerSecond = 0.8F;

private:
    mage::renderer::MeshHandle    m_cubeMesh{};
    mage::renderer::TextureHandle m_orangeTexture{};
    mage::renderer::TextureHandle m_blueTexture{};
    mage::renderer::TextureHandle m_groundTexture{};

    float m_spinAngleInRadians = 0.0F;

    std::array<mage::renderer::DrawCommand, 3> m_drawCommands;
};
