#include "vulkan/MeshPipeline.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanShaderModule.hpp"

#include <mage/core/Log.hpp>
#include <mage/renderer/Mesh.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace mage::renderer
{
    namespace
    {
        // Names of the functions marked [shader("...")] in mesh.slang.
        constexpr const char* VERTEX_ENTRY_POINT = "vertexMain";
        constexpr const char* FRAGMENT_ENTRY_POINT = "fragmentMain";
    } // namespace

    MeshPipeline::MeshPipeline(const VulkanDevice&          device,
                               vk::Format                   colorAttachmentFormat,
                               vk::Format                   depthAttachmentFormat,
                               const std::filesystem::path& shaderDirectory)
    {
        const vk::raii::Device& logicalDevice = device.logicalDevice();

        // --- Descriptor set layouts ---------------------------------------------------
        // Set 0, binding 0: one uniform buffer, read by both shader stages.
        const vk::DescriptorSetLayoutBinding frameUniformsBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        };

        m_frameDescriptorSetLayout = logicalDevice.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
            .bindingCount = 1,
            .pBindings = &frameUniformsBinding,
        });

        // Set 1, binding 0: one texture with its sampler, read by the fragment shader.
        const vk::DescriptorSetLayoutBinding albedoTextureBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        };

        m_materialDescriptorSetLayout = logicalDevice.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{
            .bindingCount = 1,
            .pBindings = &albedoTextureBinding,
        });

        // --- Pipeline layout ----------------------------------------------------------
        // The index of a layout in this array is its set number in the shader.
        const std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = {
            *m_frameDescriptorSetLayout,    // FRAME_DESCRIPTOR_SET_INDEX
            *m_materialDescriptorSetLayout, // MATERIAL_DESCRIPTOR_SET_INDEX
        };

        const vk::PushConstantRange pushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset = 0,
            .size = sizeof(ObjectPushConstants),
        };

        m_pipelineLayout = logicalDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{
            .setLayoutCount = static_cast<std::uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange,
        });

        // --- Shader stages ------------------------------------------------------------
        // The shader module is only needed while the pipeline is being created:
        // it is destroyed at the end of this constructor.
        const vk::raii::ShaderModule shaderModule = loadShaderModule(logicalDevice, shaderDirectory / "mesh.spv");

        // One SPIR-V module contains both entry points; each stage names its own.
        const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
            vk::PipelineShaderStageCreateInfo{
                .stage = vk::ShaderStageFlagBits::eVertex,
                .module = *shaderModule,
                .pName = VERTEX_ENTRY_POINT,
            },
            vk::PipelineShaderStageCreateInfo{
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = *shaderModule,
                .pName = FRAGMENT_ENTRY_POINT,
            },
        };

        // --- Vertex input -------------------------------------------------------------
        // Tells the pipeline how to read the vertex buffer.
        //
        // A BINDING describes one vertex buffer as a whole: here a single
        // buffer (binding 0) in which every vertex occupies sizeof(Vertex) bytes.
        const vk::VertexInputBindingDescription vertexBinding{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex, // Advance once per vertex (not once per instance).
        };

        // An ATTRIBUTE describes one field of a vertex: which shader input
        // location receives it, its data format, and where it starts inside
        // the vertex. Formats are named like colour formats: three 32-bit
        // floats is "R32G32B32Sfloat".
        const std::array<vk::VertexInputAttributeDescription, 3> vertexAttributes = {
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, position),
            },
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, normal),
            },
            vk::VertexInputAttributeDescription{
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(Vertex, textureCoordinates),
            },
        };

        const vk::PipelineVertexInputStateCreateInfo vertexInputState{
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBinding,
            .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexAttributes.size()),
            .pVertexAttributeDescriptions = vertexAttributes.data(),
        };

        // --- Input assembly -----------------------------------------------------------
        // How vertices are grouped into primitives: every 3 indices form one
        // independent triangle.
        const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };

        // --- Viewport and scissor -----------------------------------------------------
        // The viewport maps normalized coordinates to pixels; the scissor is
        // a clipping rectangle. Only their COUNT is fixed here: the actual
        // rectangles are "dynamic state" (see below), set while recording.
        const vk::PipelineViewportStateCreateInfo viewportState{
            .viewportCount = 1,
            .scissorCount = 1,
        };

        // --- Rasterization ------------------------------------------------------------
        // Turns each triangle into fragments (candidate pixels).
        const vk::PipelineRasterizationStateCreateInfo rasterizationState{
            .polygonMode = vk::PolygonMode::eFill, // Fill triangles (as opposed to wireframe).

            // Back-face culling: triangles facing away from the camera are
            // discarded, which halves the work for closed objects. MAGE meshes
            // list the vertices of a front face in counter-clockwise order.
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,

            .lineWidth = 1.0F, // Must be 1 unless the wideLines feature is enabled.
        };

        // --- Multisampling ------------------------------------------------------------
        // One sample per pixel: no multisample anti-aliasing.
        const vk::PipelineMultisampleStateCreateInfo multisampleState{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
        };

        // --- Depth test ---------------------------------------------------------------
        // The DEPTH BUFFER stores, for every pixel, the distance of the
        // closest surface drawn so far. A new fragment is kept only if it is
        // closer (depth test) and then becomes the new closest (depth write).
        // This is what makes near objects hide far ones regardless of the
        // order in which they are drawn.
        const vk::PipelineDepthStencilStateCreateInfo depthStencilState{
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLess, // Smaller depth = closer to the camera.
        };

        // --- Colour blending ----------------------------------------------------------
        // How a fragment's colour is combined with the colour already in the
        // attachment. Blending is disabled: the new colour simply replaces the
        // old one. The write mask still has to enable all four channels.
        const vk::PipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                              | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };

        // One blend state per colour attachment the pipeline renders into.
        const vk::PipelineColorBlendStateCreateInfo colorBlendState{
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
        };

        // --- Dynamic state ------------------------------------------------------------
        // State listed here is left out of the pipeline and set with commands
        // instead. Keeping viewport and scissor dynamic means the pipeline
        // does not need to be recreated when the window is resized.
        const std::array<vk::DynamicState, 2> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };

        const vk::PipelineDynamicStateCreateInfo dynamicState{
            .dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        // --- Render target formats ----------------------------------------------------
        // With dynamic rendering there is no VkRenderPass object: the pipeline
        // is only told the formats of the attachments it will be used with.
        // This structure is chained to the pipeline create info through pNext.
        const vk::PipelineRenderingCreateInfo renderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorAttachmentFormat,
            .depthAttachmentFormat = depthAttachmentFormat,
        };

        // --- Create the pipeline ------------------------------------------------------
        const vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
            .pNext = &renderingCreateInfo,
            .stageCount = static_cast<std::uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = *m_pipelineLayout,
        };

        // The first argument is an optional pipeline cache, which stores
        // compiled pipelines to speed up later creations. Not used yet.
        m_pipeline = logicalDevice.createGraphicsPipeline(nullptr, pipelineCreateInfo);

        log::info("Created mesh graphics pipeline");
    }

    vk::DescriptorSetLayout MeshPipeline::frameDescriptorSetLayout() const
    {
        return *m_frameDescriptorSetLayout;
    }

    vk::DescriptorSetLayout MeshPipeline::materialDescriptorSetLayout() const
    {
        return *m_materialDescriptorSetLayout;
    }

    vk::PipelineLayout MeshPipeline::layout() const
    {
        return *m_pipelineLayout;
    }

    void MeshPipeline::bind(const vk::raii::CommandBuffer& commandBuffer) const
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    }
} // namespace mage::renderer
