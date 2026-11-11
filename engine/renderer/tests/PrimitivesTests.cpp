#include <mage/core/Math.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Primitives.hpp>
#include <mage/renderer/Texture.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

TEST_CASE("The cube mesh has four vertices and two triangles per face", "[renderer][primitives]")
{
    const mage::renderer::MeshData cube = mage::renderer::makeCubeMesh();

    CHECK(cube.vertices.size() == 24);
    CHECK(cube.indices.size() == 36);

    for (const std::uint32_t index : cube.indices)
    {
        CHECK(index < cube.vertices.size());
    }
}

TEST_CASE("Every cube triangle is counter-clockwise seen from outside", "[renderer][primitives]")
{
    const mage::renderer::MeshData cube = mage::renderer::makeCubeMesh();

    for (std::size_t firstIndex = 0; firstIndex < cube.indices.size(); firstIndex += 3)
    {
        const mage::renderer::Vertex& vertexA = cube.vertices[cube.indices[firstIndex]];
        const mage::renderer::Vertex& vertexB = cube.vertices[cube.indices[firstIndex + 1]];
        const mage::renderer::Vertex& vertexC = cube.vertices[cube.indices[firstIndex + 2]];

        // For a counter-clockwise triangle, the cross product of two of its
        // edges points toward the viewer: the same way as the vertex normal.
        const glm::vec3 geometricNormal =
            glm::normalize(glm::cross(vertexB.position - vertexA.position, vertexC.position - vertexA.position));

        CHECK(glm::dot(geometricNormal, vertexA.normal) == Catch::Approx(1.0F));

        // The cube is centred on the origin, so "outside" is away from it.
        CHECK(glm::dot(vertexA.normal, vertexA.position) > 0.0F);
    }
}

TEST_CASE("The checkerboard texture alternates between its two colours", "[renderer][primitives]")
{
    constexpr mage::renderer::Color8 WHITE = {255, 255, 255, 255};
    constexpr mage::renderer::Color8 BLACK = {0, 0, 0, 255};

    const mage::renderer::TextureData texture = mage::renderer::makeCheckerboardTexture(2, 4, WHITE, BLACK);

    REQUIRE(texture.width == 8);
    REQUIRE(texture.height == 8);
    REQUIRE(texture.rgbaPixels.size() == 8 * 8 * 4);

    // Red channel of the pixel at the given coordinates.
    const auto redAt = [&texture](std::size_t pixelX, std::size_t pixelY)
    {
        return texture.rgbaPixels[(pixelY * texture.width + pixelX) * 4];
    };

    CHECK(redAt(0, 0) == 255); // top-left cell: first colour
    CHECK(redAt(4, 0) == 0);   // top-right cell: second colour
    CHECK(redAt(0, 4) == 0);   // bottom-left cell: second colour
    CHECK(redAt(4, 4) == 255); // bottom-right cell: first colour
}
