#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mage::platform
{
    class Window;
}

namespace mage::renderer
{
    // The Vulkan classes are private to the renderer module (engine/renderer/src/vulkan).
    // Forward declaring them keeps every Vulkan header out of the code that
    // only uses the renderer.
    class TrianglePipeline;
    class VulkanDevice;
    class VulkanFrame;
    class VulkanInstance;
    class VulkanSurface;
    class VulkanSwapchain;

    struct RendererDescription
    {
        std::string applicationName = "MAGE Application";
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

        // Renders one frame and presents it to the window. Does nothing while
        // the window is minimised.
        void drawFrame();

    private:
        // Number of frames the CPU may prepare while the GPU is still working
        // on earlier ones. Two keeps the CPU and GPU both busy without adding
        // more input latency.
        static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        void recordCommands(const VulkanFrame& frame, std::uint32_t swapchainImageIndex) const;
        void recreateSwapchain();

        const platform::Window& m_window;

        std::unique_ptr<VulkanInstance>   m_instance;         // 1. connection to the Vulkan library
        std::unique_ptr<VulkanSurface>    m_surface;          // 2. the window, as seen by Vulkan
        std::unique_ptr<VulkanDevice>     m_device;           // 3. the GPU
        std::unique_ptr<VulkanSwapchain>  m_swapchain;        // 4. the images shown in the window
        std::unique_ptr<TrianglePipeline> m_trianglePipeline; // 5. how to draw
        std::vector<VulkanFrame>          m_frames;           // 6. per-frame command buffers and synchronisation

        // Index into m_frames of the frame being prepared; cycles 0, 1, 0, 1...
        std::uint32_t m_currentFrameIndex = 0;
    };
} // namespace mage::renderer
