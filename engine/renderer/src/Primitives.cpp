#include <mage/core/Math.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Primitives.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace mage::renderer
{
    namespace
    {
        // A cube face is described by three perpendicular unit vectors. Seen
        // from outside the cube, `right` and `up` are the directions of the
        // face's own horizontal and vertical axes, and `normal` points toward
        // the viewer (right x up = normal).
        struct CubeFace
        {
            glm::vec3 normal;
            glm::vec3 right;
            glm::vec3 up;
        };

        constexpr std::array<CubeFace, 6> CUBE_FACES = {
            CubeFace{.normal = {1.0F, 0.0F, 0.0F}, .right = {0.0F, 0.0F, -1.0F}, .up = {0.0F, 1.0F, 0.0F}},  // +X
            CubeFace{.normal = {-1.0F, 0.0F, 0.0F}, .right = {0.0F, 0.0F, 1.0F}, .up = {0.0F, 1.0F, 0.0F}},  // -X
            CubeFace{.normal = {0.0F, 1.0F, 0.0F}, .right = {1.0F, 0.0F, 0.0F}, .up = {0.0F, 0.0F, -1.0F}},  // +Y
            CubeFace{.normal = {0.0F, -1.0F, 0.0F}, .right = {1.0F, 0.0F, 0.0F}, .up = {0.0F, 0.0F, 1.0F}},  // -Y
            CubeFace{.normal = {0.0F, 0.0F, 1.0F}, .right = {1.0F, 0.0F, 0.0F}, .up = {0.0F, 1.0F, 0.0F}},   // +Z
            CubeFace{.normal = {0.0F, 0.0F, -1.0F}, .right = {-1.0F, 0.0F, 0.0F}, .up = {0.0F, 1.0F, 0.0F}}, // -Z
        };
    } // namespace

    MeshData makeCubeMesh()
    {
        constexpr float HALF_EDGE_LENGTH = 0.5F;

        MeshData cube;
        cube.vertices.reserve(CUBE_FACES.size() * 4);
        cube.indices.reserve(CUBE_FACES.size() * 6);

        for (const CubeFace& face : CUBE_FACES)
        {
            const auto firstVertexIndex = static_cast<std::uint32_t>(cube.vertices.size());

            const glm::vec3 faceCenter = face.normal * HALF_EDGE_LENGTH;
            const glm::vec3 halfRight = face.right * HALF_EDGE_LENGTH;
            const glm::vec3 halfUp = face.up * HALF_EDGE_LENGTH;

            // The four corners, counter-clockwise as seen from outside:
            //
            //   3 --- 2        texture coordinates: (0,0) --- (1,0)
            //   |     |                               |         |
            //   0 --- 1                             (0,1) --- (1,1)
            cube.vertices.push_back(Vertex{
                .position = faceCenter - halfRight - halfUp,
                .normal = face.normal,
                .textureCoordinates = {0.0F, 1.0F},
            });
            cube.vertices.push_back(Vertex{
                .position = faceCenter + halfRight - halfUp,
                .normal = face.normal,
                .textureCoordinates = {1.0F, 1.0F},
            });
            cube.vertices.push_back(Vertex{
                .position = faceCenter + halfRight + halfUp,
                .normal = face.normal,
                .textureCoordinates = {1.0F, 0.0F},
            });
            cube.vertices.push_back(Vertex{
                .position = faceCenter - halfRight + halfUp,
                .normal = face.normal,
                .textureCoordinates = {0.0F, 0.0F},
            });

            // Two triangles per face: 0-1-2 and 0-2-3.
            for (const std::uint32_t cornerIndex : {0U, 1U, 2U, 0U, 2U, 3U})
            {
                cube.indices.push_back(firstVertexIndex + cornerIndex);
            }
        }

        return cube;
    }

    TextureData makeCheckerboardTexture(std::uint32_t cellsPerSide,
                                        std::uint32_t cellSizeInPixels,
                                        Color8        firstColor,
                                        Color8        secondColor)
    {
        const std::uint32_t sideInPixels = cellsPerSide * cellSizeInPixels;

        TextureData texture;
        texture.width = sideInPixels;
        texture.height = sideInPixels;
        texture.rgbaPixels.reserve(static_cast<std::size_t>(sideInPixels) * sideInPixels * 4);

        for (std::uint32_t pixelY = 0; pixelY < sideInPixels; ++pixelY)
        {
            for (std::uint32_t pixelX = 0; pixelX < sideInPixels; ++pixelX)
            {
                const std::uint32_t cellX = pixelX / cellSizeInPixels;
                const std::uint32_t cellY = pixelY / cellSizeInPixels;

                // Cells whose coordinates have the same parity share a colour.
                const Color8& color = ((cellX + cellY) % 2 == 0) ? firstColor : secondColor;
                texture.rgbaPixels.insert(texture.rgbaPixels.end(), color.begin(), color.end());
            }
        }

        return texture;
    }
} // namespace mage::renderer
