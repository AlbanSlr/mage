#pragma once

#include <mage/core/Math.hpp>

#include <cstdint>
#include <vector>

namespace mage::renderer
{
    // One corner of a triangle, exactly as it is stored in the GPU vertex
    // buffer. The pipeline's vertex input description (MeshPipeline.cpp) and
    // the shader's VertexInput structure (mesh.slang) must match this layout.
    struct Vertex
    {
        glm::vec3 position{0.0F};

        // Unit vector perpendicular to the surface, used for lighting.
        glm::vec3 normal{0.0F, 1.0F, 0.0F};

        // Where this vertex is on the texture: (0, 0) is the top-left corner
        // of the image and (1, 1) the bottom-right one.
        glm::vec2 textureCoordinates{0.0F};
    };

    // Geometry on the CPU side, ready to be uploaded with Renderer::createMesh().
    //
    // Triangles are INDEXED: `indices` lists the vertices three by three, so a
    // vertex shared by several triangles is stored once. The front face of a
    // triangle is the one from which its vertices appear in counter-clockwise
    // order; back faces are not drawn.
    struct MeshData
    {
        std::vector<Vertex>        vertices;
        std::vector<std::uint32_t> indices;
    };

    // Identifies a mesh that lives on the GPU. Returned by Renderer::createMesh().
    enum class MeshHandle : std::uint32_t
    {
    };
} // namespace mage::renderer
