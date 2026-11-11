#pragma once

#include <mage/core/Math.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Camera.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Texture.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace mage::renderer
{
    // The Vulkan classes are private to the renderer module (engine/renderer/src).
    // Forward declaring them keeps every Vulkan header out of the code that
    // only uses the renderer.
    class MeshPipeline;
    class RenderResources;
    class SceneRenderTarget;
    class VulkanAllocator;
    class VulkanDevice;
    class VulkanFrame;
    class VulkanImage;
    class VulkanImmediateCommands;
    class VulkanInstance;
    class VulkanSurface;
    class VulkanSwapchain;
    class VulkanUserInterfaceRenderer;

    struct RendererDescription
    {
        std::string applicationName = "MAGE Application";

        // false: the scene is rendered directly into the window (a game).
        //
        // true: the scene is rendered into an offscreen image, and the window
        // shows the Dear ImGui user interface, which can display that image
        // in one of its panels (the editor). A ui::UserInterface must exist
        // before the renderer is created and must outlive it.
        bool enableUserInterface = false;
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

        // --- User interface mode ---------------------------------------------------
        // Only valid when the renderer was created with `enableUserInterface`.

        // Must be called once per frame BEFORE the interface is built
        // (ui::UserInterface::beginFrame). Applies a pending change of the
        // scene viewport size.
        void beginUserInterfaceFrame();

        // Requests the size, in pixels, of the offscreen image the scene is
        // rendered into: typically the size of the panel that displays it.
        // Takes effect at the next beginUserInterfaceFrame().
        void setSceneViewportExtent(platform::PixelExtent extent);

        [[nodiscard]] platform::PixelExtent sceneViewportExtent() const;

        // Identifier of the scene image for Dear ImGui:
        //     ImGui::Image(ImTextureRef(renderer.sceneViewportTextureId()), size);
        // It changes when the viewport is resized, so it must be queried every
        // frame after beginUserInterfaceFrame().
        [[nodiscard]] std::uint64_t sceneViewportTextureId() const;

    private:
        // Number of frames the CPU may prepare while the GPU is still working
        // on earlier ones. Two keeps the CPU and GPU both busy without adding
        // more input latency.
        static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        [[nodiscard]] bool isUserInterfaceEnabled() const;

        void updateFrameUniforms(const VulkanFrame& frame, const FrameDescription& frameDescription) const;
        void recordCommands(const VulkanFrame&      frame,
                            std::uint32_t           swapchainImageIndex,
                            const FrameDescription& frameDescription) const;
        void createDepthImage();
        void createSceneRenderTarget(platform::PixelExtent extent);
        void recreateSwapchain();

        const platform::Window& m_window;

        std::unique_ptr<VulkanInstance>          m_instance;          //  1. connection to the Vulkan library
        std::unique_ptr<VulkanSurface>           m_surface;           //  2. the window, as seen by Vulkan
        std::unique_ptr<VulkanDevice>            m_device;            //  3. the GPU
        std::unique_ptr<VulkanAllocator>         m_allocator;         //  4. GPU memory
        std::unique_ptr<VulkanImmediateCommands> m_immediateCommands; //  5. one-off GPU work (uploads)
        std::unique_ptr<VulkanSwapchain>         m_swapchain;         //  6. the images shown in the window

        // 7. Where the scene is rendered. Exactly one of the two exists:
        //    - direct mode: into the swapchain images, with this depth buffer;
        //    - user interface mode: into an offscreen colour + depth target.
        std::unique_ptr<VulkanImage>       m_depthImage;
        std::unique_ptr<SceneRenderTarget> m_sceneRenderTarget;

        std::unique_ptr<MeshPipeline>    m_meshPipeline; //  8. how to draw
        std::unique_ptr<RenderResources> m_resources;    //  9. meshes, textures, descriptor pool
        std::vector<VulkanFrame>         m_frames;       // 10. per-frame commands, sync and uniforms

        // 11. Draws Dear ImGui. Null in direct mode.
        std::unique_ptr<VulkanUserInterfaceRenderer> m_userInterfaceRenderer;

        // Identifier under which the scene render target is registered with
        // the user interface renderer.
        std::uint64_t m_sceneViewportTextureId = 0;

        // Viewport size requested by setSceneViewportExtent(), applied at the
        // next beginUserInterfaceFrame().
        platform::PixelExtent m_requestedSceneViewportExtent;

        // Index into m_frames of the frame being prepared; cycles 0, 1, 0, 1...
        std::uint32_t m_currentFrameIndex = 0;
    };
} // namespace mage::renderer
