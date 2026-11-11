#pragma once

#include "vulkan/VulkanApi.hpp"

#include <mage/core/Math.hpp>

#include <filesystem>

namespace mage::renderer
{
    class VulkanDevice;

    // Data shared by every object of a frame. Uploaded once per frame into a
    // uniform buffer bound at descriptor set 0, binding 0.
    // Must match `FrameUniforms` in shaders/mesh.slang.
    struct FrameUniforms
    {
        glm::mat4 viewProjectionMatrix{1.0F};
        glm::vec4 directionToLight{0.0F, 1.0F, 0.0F, 0.0F};
    };

    // Data specific to one draw call, sent as push constants.
    // Must match `ObjectConstants` in shaders/mesh.slang.
    //
    // 128 bytes: exactly the push constant size every Vulkan implementation is
    // guaranteed to support.
    struct ObjectPushConstants
    {
        glm::mat4 modelMatrix{1.0F};
        glm::mat4 normalMatrix{1.0F};
    };

    // The graphics pipeline drawing textured, lit, depth-tested meshes, and
    // the description of the resources its shaders use.
    //
    // DESCRIPTORS are how shaders reach resources that live outside of them:
    //   - a DESCRIPTOR is a reference to one resource (a uniform buffer, a
    //     texture with its sampler...);
    //   - a DESCRIPTOR SET is a group of descriptors bound together;
    //   - a DESCRIPTOR SET LAYOUT is the "type" of a set: which kind of
    //     descriptor sits at which binding number;
    //   - the PIPELINE LAYOUT lists the set layouts (and push constants) a
    //     pipeline expects.
    //
    // This pipeline uses two sets, split by how often their content changes:
    //   set 0  per frame     binding 0: uniform buffer (FrameUniforms)
    //   set 1  per material  binding 0: texture + sampler
    class MeshPipeline
    {
    public:
        static constexpr std::uint32_t FRAME_DESCRIPTOR_SET_INDEX = 0;
        static constexpr std::uint32_t MATERIAL_DESCRIPTOR_SET_INDEX = 1;

        // The formats are those of the images the pipeline will render into.
        // `shaderDirectory` is the folder containing the compiled `mesh.spv`.
        MeshPipeline(const VulkanDevice&          device,
                     vk::Format                   colorAttachmentFormat,
                     vk::Format                   depthAttachmentFormat,
                     const std::filesystem::path& shaderDirectory);

        [[nodiscard]] vk::DescriptorSetLayout frameDescriptorSetLayout() const;
        [[nodiscard]] vk::DescriptorSetLayout materialDescriptorSetLayout() const;
        [[nodiscard]] vk::PipelineLayout layout() const;

        // Makes this pipeline the one used by the following draw commands.
        void bind(const vk::raii::CommandBuffer& commandBuffer) const;

    private:
        // Declared in creation order (destroyed in reverse).
        vk::raii::DescriptorSetLayout m_frameDescriptorSetLayout = nullptr;
        vk::raii::DescriptorSetLayout m_materialDescriptorSetLayout = nullptr;
        vk::raii::PipelineLayout      m_pipelineLayout = nullptr;
        vk::raii::Pipeline            m_pipeline = nullptr;
    };
} // namespace mage::renderer
