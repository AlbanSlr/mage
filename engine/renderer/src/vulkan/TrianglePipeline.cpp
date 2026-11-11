#include "vulkan/TrianglePipeline.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"

#include <mage/core/File.hpp>
#include <mage/core/Log.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <vector>

namespace mage::renderer
{
    namespace
    {
        // Names of the functions marked [shader("...")] in triangle.slang.
        constexpr const char* VERTEX_ENTRY_POINT = "vertexMain";
        constexpr const char* FRAGMENT_ENTRY_POINT = "fragmentMain";

        // Loads a compiled shader file and wraps it in a VkShaderModule, the
        // object that hands SPIR-V code to the driver.
        vk::raii::ShaderModule loadShaderModule(const vk::raii::Device&      logicalDevice,
                                                const std::filesystem::path& filePath)
        {
            const std::vector<std::byte> fileContent = readBinaryFile(filePath);

            // SPIR-V is a stream of 32-bit words, and Vulkan wants it as such.
            if (fileContent.empty() || fileContent.size() % sizeof(std::uint32_t) != 0)
            {
                throw std::runtime_error(std::format("'{}' is not a valid SPIR-V file", filePath.string()));
            }

            // Copy into uint32_t storage so the data is correctly aligned.
            std::vector<std::uint32_t> spirvWords(fileContent.size() / sizeof(std::uint32_t));
            std::memcpy(spirvWords.data(), fileContent.data(), fileContent.size());

            const vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
                .codeSize = fileContent.size(), // In bytes, not in words.
                .pCode = spirvWords.data(),
            };

            return logicalDevice.createShaderModule(shaderModuleCreateInfo);
        }
    } // namespace

    TrianglePipeline::TrianglePipeline(const VulkanDevice&          device,
                                       vk::Format                   colorAttachmentFormat,
                                       const std::filesystem::path& shaderDirectory)
    {
        const vk::raii::Device& logicalDevice = device.logicalDevice();

        // --- Shader stages ---------------------------------------------------------
        // The shader module is only needed while the pipeline is being created:
        // it is destroyed at the end of this constructor.
        const vk::raii::ShaderModule shaderModule = loadShaderModule(logicalDevice, shaderDirectory / "triangle.spv");

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

        // --- Vertex input ----------------------------------------------------------
        // Describes the layout of vertex buffers. Empty: the triangle's
        // vertices are generated inside the vertex shader.
        const vk::PipelineVertexInputStateCreateInfo vertexInputState{};

        // --- Input assembly --------------------------------------------------------
        // How vertices are grouped into primitives: every 3 vertices form one
        // independent triangle.
        const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
            .topology = vk::PrimitiveTopology::eTriangleList,
        };

        // --- Viewport and scissor --------------------------------------------------
        // The viewport maps normalized coordinates to pixels; the scissor is
        // a clipping rectangle. Only their COUNT is fixed here: the actual
        // rectangles are "dynamic state" (see below), set while recording.
        const vk::PipelineViewportStateCreateInfo viewportState{
            .viewportCount = 1,
            .scissorCount = 1,
        };

        // --- Rasterization ---------------------------------------------------------
        // Turns each triangle into fragments (candidate pixels).
        const vk::PipelineRasterizationStateCreateInfo rasterizationState{
            .polygonMode = vk::PolygonMode::eFill, // Fill triangles (as opposed to wireframe).

            // Back-face culling: triangles facing away from the camera are
            // discarded. A triangle is front-facing when its vertices appear
            // in clockwise order on screen.
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,

            .lineWidth = 1.0F, // Must be 1 unless the wideLines feature is enabled.
        };

        // --- Multisampling ---------------------------------------------------------
        // One sample per pixel: no multisample anti-aliasing.
        const vk::PipelineMultisampleStateCreateInfo multisampleState{
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
        };

        // --- Colour blending -------------------------------------------------------
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

        // --- Dynamic state ---------------------------------------------------------
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

        // --- Pipeline layout -------------------------------------------------------
        m_pipelineLayout = logicalDevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{});

        // --- Render target formats -------------------------------------------------
        // With dynamic rendering there is no VkRenderPass object: the pipeline
        // is only told the formats of the attachments it will be used with.
        // This structure is chained to the pipeline create info through pNext.
        const vk::PipelineRenderingCreateInfo renderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &colorAttachmentFormat,
        };

        // --- Create the pipeline ---------------------------------------------------
        const vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
            .pNext = &renderingCreateInfo,
            .stageCount = static_cast<std::uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = *m_pipelineLayout,
        };

        // The second argument is an optional pipeline cache, which stores
        // compiled pipelines to speed up later creations. Not used yet.
        m_pipeline = logicalDevice.createGraphicsPipeline(nullptr, pipelineCreateInfo);

        log::info("Created triangle graphics pipeline");
    }

    void TrianglePipeline::draw(const vk::raii::CommandBuffer& commandBuffer) const
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);

        // 3 vertices, 1 instance, starting at vertex 0 and instance 0.
        commandBuffer.draw(3, 1, 0, 0);
    }
} // namespace mage::renderer
