#pragma once

#include "vulkan/VulkanApi.hpp"

#include <cstdint>

namespace mage::renderer
{
    class VulkanDevice;
    class VulkanInstance;

    // Draws the Dear ImGui user interface with Vulkan.
    //
    // This wraps Dear ImGui's own Vulkan "renderer backend"
    // (imgui_impl_vulkan), which owns the pipeline, the font texture and the
    // vertex buffers needed to draw the interface. MAGE only has to initialise
    // it and call it at the right place in the frame.
    //
    // A Dear ImGui context must exist before this object is created, and must
    // outlive it (see ui::UserInterface).
    class VulkanUserInterfaceRenderer
    {
    public:
        // `colorAttachmentFormat` and `swapchainImageCount` describe the
        // swapchain the interface is drawn into.
        VulkanUserInterfaceRenderer(const VulkanInstance& instance,
                                    const VulkanDevice&   device,
                                    vk::Format            colorAttachmentFormat,
                                    std::uint32_t         swapchainImageCount);
        ~VulkanUserInterfaceRenderer();

        VulkanUserInterfaceRenderer(const VulkanUserInterfaceRenderer&) = delete;
        VulkanUserInterfaceRenderer& operator=(const VulkanUserInterfaceRenderer&) = delete;
        VulkanUserInterfaceRenderer(VulkanUserInterfaceRenderer&&) = delete;
        VulkanUserInterfaceRenderer& operator=(VulkanUserInterfaceRenderer&&) = delete;

        // Must be called once per frame before the interface is built.
        void beginFrame() const;

        // Records the commands drawing the interface built this frame. Must
        // be called between beginRendering() and endRendering(). Does nothing
        // if no interface was built.
        void render(const vk::raii::CommandBuffer& commandBuffer) const;

        // Makes an image displayable by the interface (ImGui::Image). The
        // image must be in the ShaderReadOnlyOptimal layout whenever the
        // interface is drawn. Returns the identifier to give to Dear ImGui.
        [[nodiscard]] std::uint64_t registerTexture(vk::ImageView imageView) const;

        void unregisterTexture(std::uint64_t textureId) const;

    private:
        // Dear ImGui keeps a pointer to the format: it must stay alive.
        vk::Format m_colorAttachmentFormat;
    };
} // namespace mage::renderer
