#pragma once

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanImage.hpp"

#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Texture.hpp>

#include <cstdint>
#include <vector>

namespace mage::renderer
{
    class VulkanAllocator;
    class VulkanDevice;
    class VulkanImmediateCommands;

    // A mesh as the GPU sees it.
    struct GpuMesh
    {
        VulkanBuffer  vertexBuffer;
        VulkanBuffer  indexBuffer;
        std::uint32_t indexCount = 0;
    };

    // A texture as the GPU sees it, with the descriptor set that makes it
    // available to shaders (set 1 of the mesh pipeline).
    struct GpuTexture
    {
        // Declared before the descriptor set, which refers to it.
        VulkanImage             image;
        vk::raii::DescriptorSet materialDescriptorSet = nullptr;
    };

    // Owns the GPU-side meshes and textures, and the objects they share.
    //
    // The public handles (MeshHandle, TextureHandle) are indices into the
    // arrays stored here. Resources live until the renderer is destroyed;
    // releasing individual resources comes with the asset system.
    class RenderResources
    {
    public:
        // Upper bounds used to size the descriptor pool.
        static constexpr std::uint32_t MAX_TEXTURE_COUNT = 256;
        static constexpr std::uint32_t MAX_FRAME_UNIFORM_COUNT = 8;

        // `materialDescriptorSetLayout` is the layout of the descriptor set
        // created for each texture.
        RenderResources(const VulkanDevice&            device,
                        const VulkanAllocator&         allocator,
                        const VulkanImmediateCommands& immediateCommands,
                        vk::DescriptorSetLayout        materialDescriptorSetLayout);

        // Upload CPU-side data to the GPU. Both block until the copy is done.
        [[nodiscard]] MeshHandle createMesh(const MeshData& meshData);
        [[nodiscard]] TextureHandle createTexture(const TextureData& textureData);

        [[nodiscard]] const GpuMesh& mesh(MeshHandle meshHandle) const;
        [[nodiscard]] const GpuTexture& texture(TextureHandle textureHandle) const;

        // The pool every descriptor set of the renderer is allocated from.
        [[nodiscard]] const vk::raii::DescriptorPool& descriptorPool() const;

    private:
        const VulkanDevice&            m_device;
        const VulkanAllocator&         m_allocator;
        const VulkanImmediateCommands& m_immediateCommands;
        vk::DescriptorSetLayout        m_materialDescriptorSetLayout;

        // Descriptor sets are not created individually: they are allocated
        // from a DESCRIPTOR POOL, which is created with a fixed capacity.
        // Declared before the textures, whose descriptor sets come from it.
        vk::raii::DescriptorPool m_descriptorPool = nullptr;

        // A SAMPLER describes how a texture is read: how to blend between
        // pixels (filtering) and what happens outside of the 0..1 range
        // (addressing). One sampler is shared by all textures for now.
        vk::raii::Sampler m_defaultSampler = nullptr;

        std::vector<GpuMesh>    m_meshes;
        std::vector<GpuTexture> m_textures;
    };
} // namespace mage::renderer
