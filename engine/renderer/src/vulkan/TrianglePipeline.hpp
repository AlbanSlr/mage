#pragma once

#include "vulkan/VulkanApi.hpp"

#include <filesystem>

namespace mage::renderer
{
    class VulkanDevice;

    // The graphics pipeline drawing the first triangle.
    //
    // A GRAPHICS PIPELINE is the complete, pre-compiled description of how
    // vertices become pixels: which shaders run, how triangles are assembled
    // and rasterized, how colours are blended into the target... In Vulkan
    // almost all of this state is fixed when the pipeline is created, which is
    // what lets the driver optimise it ahead of time. Drawing differently
    // means binding a different pipeline.
    //
    // This class is a stepping stone: it hard-codes one shader and one state
    // setup, and will be replaced by a general pipeline/material abstraction
    // once vertex buffers and textures exist.
    class TrianglePipeline
    {
    public:
        // `colorAttachmentFormat` is the format of the images the pipeline
        // will render into (the swapchain format). `shaderDirectory` is the
        // folder containing the compiled `triangle.spv`.
        TrianglePipeline(const VulkanDevice&          device,
                         vk::Format                   colorAttachmentFormat,
                         const std::filesystem::path& shaderDirectory);

        // Records the commands drawing the triangle. Must be called between
        // beginRendering() and endRendering(), with viewport and scissor set.
        void draw(const vk::raii::CommandBuffer& commandBuffer) const;

    private:
        // The pipeline layout describes the resources (textures, uniform
        // buffers, push constants) shaders can access. The triangle uses none,
        // but a layout is still mandatory.
        vk::raii::PipelineLayout m_pipelineLayout = nullptr;
        vk::raii::Pipeline       m_pipeline = nullptr;
    };
} // namespace mage::renderer
