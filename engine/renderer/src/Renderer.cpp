#include "vulkan/TrianglePipeline.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanFrame.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSurface.hpp"
#include "vulkan/VulkanSwapchain.hpp"

#include <mage/core/Log.hpp>
#include <mage/platform/FileSystem.hpp>
#include <mage/platform/VulkanWindowSurface.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Renderer.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace mage::renderer
{
    namespace
    {
#if defined(MAGE_ENABLE_VULKAN_VALIDATION)
        constexpr bool ENABLE_VULKAN_VALIDATION = true;
#else
        constexpr bool ENABLE_VULKAN_VALIDATION = false;
#endif

        // Colour the window is cleared to before drawing (linear RGB + alpha).
        constexpr std::array<float, 4> CLEAR_COLOR = {0.02F, 0.02F, 0.03F, 1.0F};

        vk::Extent2D toVulkanExtent(platform::PixelExtent extent)
        {
            return vk::Extent2D{.width = extent.width, .height = extent.height};
        }

        // Describes one change of IMAGE LAYOUT.
        //
        // A Vulkan image is always in a "layout" that states what it is
        // currently used for (being rendered to, being presented, being
        // sampled by a shader...), and the GPU may physically organise the
        // pixel data differently for each. The application must move the image
        // to the right layout before each use, with a pipeline barrier.
        //
        // A barrier also carries synchronisation information: which earlier
        // pipeline stages and memory accesses must be finished (source), and
        // which later ones must wait for the transition (destination).
        struct ImageLayoutTransition
        {
            vk::ImageLayout         oldLayout = vk::ImageLayout::eUndefined;
            vk::ImageLayout         newLayout = vk::ImageLayout::eUndefined;
            vk::PipelineStageFlags2 sourceStage;
            vk::AccessFlags2        sourceAccess;
            vk::PipelineStageFlags2 destinationStage;
            vk::AccessFlags2        destinationAccess;
        };

        void transitionColorImageLayout(const vk::raii::CommandBuffer& commandBuffer,
                                        vk::Image                      image,
                                        const ImageLayoutTransition&   transition)
        {
            const vk::ImageMemoryBarrier2 imageBarrier{
                .srcStageMask = transition.sourceStage,
                .srcAccessMask = transition.sourceAccess,
                .dstStageMask = transition.destinationStage,
                .dstAccessMask = transition.destinationAccess,
                .oldLayout = transition.oldLayout,
                .newLayout = transition.newLayout,

                // Barriers can also transfer an image between queue families.
                // "Ignored" on both sides means no transfer.
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,

                .image = image,
                .subresourceRange =
                    vk::ImageSubresourceRange{
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            const vk::DependencyInfo dependencyInfo{
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &imageBarrier,
            };

            commandBuffer.pipelineBarrier2(dependencyInfo);
        }
    } // namespace

    Renderer::Renderer(const platform::Window& window, const RendererDescription& description)
        : m_window(window)
    {
        log::info("Initialising the Vulkan renderer");

        // The window system decides which instance extensions are needed to
        // present to a window on this platform.
        const std::vector<const char*> windowExtensions = platform::requiredVulkanInstanceExtensions(m_window);

        m_instance = std::make_unique<VulkanInstance>(VulkanInstanceDescription{
            .applicationName = description.applicationName,
            .requiredExtensions = windowExtensions,
            .enableValidation = ENABLE_VULKAN_VALIDATION,
        });

        m_surface = std::make_unique<VulkanSurface>(*m_instance, m_window);
        m_device = std::make_unique<VulkanDevice>(*m_instance, *m_surface);

        m_swapchain =
            std::make_unique<VulkanSwapchain>(*m_device, *m_surface, toVulkanExtent(m_window.framebufferExtent()));

        m_trianglePipeline = std::make_unique<TrianglePipeline>(
            *m_device, m_swapchain->imageFormat(), platform::executableDirectory() / "shaders");

        m_frames.reserve(MAX_FRAMES_IN_FLIGHT);
        for (std::uint32_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
        {
            m_frames.emplace_back(*m_device);
        }

        log::info("Vulkan renderer ready");
    }

    Renderer::~Renderer()
    {
        // Vulkan objects must not be destroyed while the GPU still uses them.
        // waitIdle() blocks until every submitted command has finished, after
        // which the members can safely be destroyed in reverse order.
        if (m_device)
        {
            m_device->logicalDevice().waitIdle();
        }
    }

    void Renderer::drawFrame()
    {
        // A minimised window has a 0x0 framebuffer, and a swapchain cannot
        // have that size: skip rendering until the window is restored.
        const vk::Extent2D windowExtent = toVulkanExtent(m_window.framebufferExtent());
        if (m_window.isMinimized() || windowExtent.width == 0 || windowExtent.height == 0)
        {
            return;
        }

        const vk::raii::Device& logicalDevice = m_device->logicalDevice();
        const VulkanFrame&      frame = m_frames[m_currentFrameIndex];

        // --- 1. Wait until this frame's resources are free ---------------------------
        // The GPU may still be executing the commands recorded the last time
        // this VulkanFrame was used (MAX_FRAMES_IN_FLIGHT frames ago).
        constexpr std::uint64_t NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();

        // With no timeout the only possible outcome is success (errors throw).
        static_cast<void>(logicalDevice.waitForFences(*frame.inFlightFence(), vk::True, NO_TIMEOUT));

        // --- 2. Make sure the swapchain matches the window ---------------------------
        if (windowExtent != m_swapchain->extent())
        {
            recreateSwapchain();
        }

        // --- 3. Acquire the swapchain image to render into ---------------------------
        const VulkanSwapchain::AcquiredImage acquiredImage =
            m_swapchain->acquireNextImage(*frame.imageAvailableSemaphore());

        if (acquiredImage.result == vk::Result::eErrorOutOfDateKHR)
        {
            // The swapchain can no longer be used (the window changed between
            // step 2 and now). Recreate it and try again next frame. The fence
            // was not reset, so the next wait in step 1 returns immediately.
            recreateSwapchain();
            return;
        }

        // The fence is only reset once it is certain that work will be
        // submitted. Resetting it earlier and then returning would leave it
        // unsignalled forever and deadlock the next wait.
        logicalDevice.resetFences(*frame.inFlightFence());

        // --- 4. Record this frame's commands -----------------------------------------
        // Resetting the pool recycles the memory of all its command buffers.
        frame.commandPool().reset();
        recordCommands(frame, acquiredImage.imageIndex);

        // --- 5. Submit the commands to the GPU ---------------------------------------
        // WAIT: do not start writing colours until the acquired image is
        // really available. Only the colour output stage waits; earlier stages
        // (vertex processing) may already run.
        const vk::SemaphoreSubmitInfo waitSemaphoreInfo{
            .semaphore = *frame.imageAvailableSemaphore(),
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        };

        const vk::CommandBufferSubmitInfo commandBufferInfo{
            .commandBuffer = *frame.commandBuffer(),
        };

        // SIGNAL: once all commands have executed, presentation may proceed.
        const vk::Semaphore renderFinishedSemaphore = m_swapchain->renderFinishedSemaphore(acquiredImage.imageIndex);

        const vk::SemaphoreSubmitInfo signalSemaphoreInfo{
            .semaphore = renderFinishedSemaphore,
            .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
        };

        const vk::SubmitInfo2 submitInfo{
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphoreInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphoreInfo,
        };

        // The fence is signalled when the GPU has finished this submission.
        m_device->graphicsQueue().submit2(submitInfo, *frame.inFlightFence());

        // --- 6. Present the image ----------------------------------------------------
        // Hands the image back to the window system, which shows it on screen
        // as soon as rendering is finished (it waits on the semaphore).
        const vk::SwapchainKHR swapchainHandle = m_swapchain->handle();

        const vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchainHandle,
            .pImageIndices = &acquiredImage.imageIndex,
        };

        const vk::Result presentResult = m_device->graphicsQueue().presentKHR(presentInfo);

        // Suboptimal: the image was presented, but the swapchain no longer
        // matches the surface exactly. Out of date: it could not be presented.
        // Either way, a new swapchain is needed for the following frames.
        if (presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR
            || acquiredImage.result == vk::Result::eSuboptimalKHR)
        {
            recreateSwapchain();
        }

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::recordCommands(const VulkanFrame& frame, std::uint32_t swapchainImageIndex) const
    {
        const vk::raii::CommandBuffer& commandBuffer = frame.commandBuffer();
        const vk::Image                swapchainImage = m_swapchain->image(swapchainImageIndex);
        const vk::Extent2D             extent = m_swapchain->extent();

        // "One time submit": this recording is submitted once, then discarded
        // when the pool is reset next frame.
        commandBuffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        // --- Prepare the image for rendering -----------------------------------------
        // Undefined -> ColorAttachmentOptimal. "Undefined" as the old layout
        // means the previous content may be discarded, which is fine because
        // the image is cleared below.
        //
        // The source stage is ColorAttachmentOutput on purpose: it is the
        // stage the submission makes wait for the image-available semaphore
        // (Renderer::drawFrame, step 5), so the transition cannot happen before
        // the image has actually been acquired.
        transitionColorImageLayout(commandBuffer,
                                   swapchainImage,
                                   ImageLayoutTransition{
                                       .oldLayout = vk::ImageLayout::eUndefined,
                                       .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                       .sourceStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                       .sourceAccess = vk::AccessFlagBits2::eNone,
                                       .destinationStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                       .destinationAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                                   });

        // --- Begin rendering ---------------------------------------------------------
        // An ATTACHMENT is an image a draw call writes to. For each one,
        // Vulkan must be told what to do with its content at the start
        // (load operation) and at the end (store operation) of rendering.
        const vk::RenderingAttachmentInfo colorAttachment{
            .imageView = m_swapchain->imageView(swapchainImageIndex),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,   // Start from the clear colour.
            .storeOp = vk::AttachmentStoreOp::eStore, // Keep the result: it is going to be displayed.
            .clearValue = vk::ClearValue(vk::ClearColorValue(CLEAR_COLOR)),
        };

        const vk::Rect2D renderArea{
            .offset = vk::Offset2D{.x = 0, .y = 0},
            .extent = extent,
        };

        const vk::RenderingInfo renderingInfo{
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
        };

        commandBuffer.beginRendering(renderingInfo);

        // --- Dynamic state -----------------------------------------------------------
        // The viewport maps normalized device coordinates (-1..1) to the whole
        // image; depth values are kept in the standard 0..1 range.
        const vk::Viewport viewport{
            .x = 0.0F,
            .y = 0.0F,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
        };
        commandBuffer.setViewport(0, viewport);

        // The scissor rectangle discards every pixel outside of it.
        commandBuffer.setScissor(0, renderArea);

        // --- Draw --------------------------------------------------------------------
        m_trianglePipeline->draw(commandBuffer);

        commandBuffer.endRendering();

        // --- Prepare the image for presentation --------------------------------------
        // ColorAttachmentOptimal -> PresentSrc, the layout the window system
        // requires. The transition waits for the colour writes above; nothing
        // later in this command buffer depends on it (the presentation is
        // synchronised by the render-finished semaphore instead).
        transitionColorImageLayout(commandBuffer,
                                   swapchainImage,
                                   ImageLayoutTransition{
                                       .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                       .newLayout = vk::ImageLayout::ePresentSrcKHR,
                                       .sourceStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                       .sourceAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                                       .destinationStage = vk::PipelineStageFlagBits2::eNone,
                                       .destinationAccess = vk::AccessFlagBits2::eNone,
                                   });

        commandBuffer.end();
    }

    void Renderer::recreateSwapchain()
    {
        const vk::Extent2D windowExtent = toVulkanExtent(m_window.framebufferExtent());
        if (windowExtent.width == 0 || windowExtent.height == 0)
        {
            return; // Minimised: drawFrame() will come back here once restored.
        }

        // The old swapchain images and views may still be referenced by
        // commands the GPU is executing. Waiting for the device to be idle is
        // the simplest way to make destroying them safe; a resize is rare
        // enough that the stall does not matter.
        m_device->logicalDevice().waitIdle();

        m_swapchain->recreate(windowExtent);
    }
} // namespace mage::renderer
