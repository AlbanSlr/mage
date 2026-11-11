#pragma once

#include <mage/core/Math.hpp>
#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Texture.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace mage::platform
{
    class Window;
}

namespace mage::renderer
{
    // The Vulkan classes are private to the renderer module (engine/renderer/src).
    // Forward declaring them keeps every Vulkan header out of the code that
    // only uses the renderer.
    class MeshPipeline;
    class RenderResources;
    class VulkanAllocator;
    class VulkanDevice;
    class VulkanFrame;
    class VulkanImage;
    class VulkanImmediateCommands;
    class VulkanInstance;
    class VulkanSurface;
    class VulkanSwapchain;

    struct RendererDescription
    {
        std::string applicationName = "MAGE Application";
    };

    // A request to draw one mesh with one texture at one place in the world.
    struct DrawCommand
    {
        MeshHandle    mesh{};
        TextureHandle texture{};

        // Local (model) space -> world space.
        glm::mat4 modelMatrix{1.0F};
    };

    // Everything the renderer needs to know to draw one frame.
    struct FrameDescription
    {
        Camera camera;

        // Direction from the scene toward the (infinitely far) light, in
        // world space. It does not need to be normalized.
        glm::vec3 directionToLight{0.3F, 1.0F, 0.5F};

        // The referenced array only needs to live for the drawFrame() call.
        std::span<const DrawCommand> drawCommands;
    };

    // Draws into a window with Vulkan.
    //
    // The renderer owns every Vulkan object. Its members, read from top to
    // bottom, are the order in which Vulkan has to be brought up; they are
    // destroyed in the opposite order.
    class Renderer
    {
    public:
        // The window must outlive the renderer.
        //
        // Throws if Vulkan cannot be initialised (no suitable GPU, missing
        // shader file, ...).
        Renderer(const platform::Window& window, const RendererDescription& description);
        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer&&) = delete;

        // Upload geometry or an image to the GPU. The returned handle is what
        // draw commands refer to; it stays valid as long as the renderer lives.
        [[nodiscard]] MeshHandle createMesh(const MeshData& meshData);
        [[nodiscard]] TextureHandle createTexture(const TextureData& textureData);

        // Renders one frame and presents it to the window. Does nothing while
        // the window is minimised.
        void drawFrame(const FrameDescription& frameDescription);

    private:
        // Number of frames the CPU may prepare while the GPU is still working
        // on earlier ones. Two keeps the CPU and GPU both busy without adding
        // more input latency.
        static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        void updateFrameUniforms(const VulkanFrame& frame, const FrameDescription& frameDescription) const;
        void recordCommands(const VulkanFrame&      frame,
                            std::uint32_t           swapchainImageIndex,
                            const FrameDescription& frameDescription) const;
        void createDepthImage();
        void recreateSwapchain();

        const platform::Window& m_window;

        std::unique_ptr<VulkanInstance>          m_instance;          //  1. connection to the Vulkan library
        std::unique_ptr<VulkanSurface>           m_surface;           //  2. the window, as seen by Vulkan
        std::unique_ptr<VulkanDevice>            m_device;            //  3. the GPU
        std::unique_ptr<VulkanAllocator>         m_allocator;         //  4. GPU memory
        std::unique_ptr<VulkanImmediateCommands> m_immediateCommands; //  5. one-off GPU work (uploads)
        std::unique_ptr<VulkanSwapchain>         m_swapchain;         //  6. the images shown in the window
        std::unique_ptr<VulkanImage>             m_depthImage;        //  7. the depth buffer, same size as those
        std::unique_ptr<MeshPipeline>            m_meshPipeline;      //  8. how to draw
        std::unique_ptr<RenderResources>         m_resources;         //  9. meshes, textures, descriptor pool
        std::vector<VulkanFrame>                 m_frames;            // 10. per-frame commands, sync and uniforms

        // Index into m_frames of the frame being prepared; cycles 0, 1, 0, 1...
        std::uint32_t m_currentFrameIndex = 0;
    };
} // namespace mage::renderer
