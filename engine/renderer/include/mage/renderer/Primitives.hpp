#pragma once

#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <cstdint>

// Procedurally generated meshes and textures: content to render before the
// engine can import asset files.
namespace mage::renderer
{
    // A cube centred on the origin with an edge length of 1. Each face has its
    // own four vertices (24 in total) so that it gets a flat normal and the
    // full texture.
    [[nodiscard]] MeshData makeCubeMesh();

    // One RGBA colour, 8 bits per channel.
    using Color8 = std::array<std::uint8_t, 4>;

    // A square texture of `cellsPerSide` x `cellsPerSide` cells alternating
    // between two colours. Each cell is `cellSizeInPixels` pixels wide.
    [[nodiscard]] TextureData makeCheckerboardTexture(std::uint32_t cellsPerSide,
                                                      std::uint32_t cellSizeInPixels,
                                                      Color8        firstColor,
                                                      Color8        secondColor);
} // namespace mage::renderer
