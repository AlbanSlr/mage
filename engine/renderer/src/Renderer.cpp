#include "RenderResources.hpp"
#include "vulkan/MeshPipeline.hpp"
#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanFrame.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/VulkanImageLayout.hpp"
#include "vulkan/VulkanImmediateCommands.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanSurface.hpp"
#include "vulkan/VulkanSwapchain.hpp"

#include <mage/core/Log.hpp>
#include <mage/core/Math.hpp>
#include <mage/platform/FileSystem.hpp>
#include <mage/platform/VulkanWindowSurface.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Mesh.hpp>
#include <mage/renderer/Renderer.hpp>
#include <mage/renderer/Texture.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
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

        // Depth the depth buffer is cleared to: 1 is the far plane, so
        // anything drawn is closer than the initial content.
        constexpr float CLEAR_DEPTH = 1.0F;

        vk::Extent2D toVulkanExtent(platform::PixelExtent extent)
        {
            return vk::Extent2D{.width = extent.width, .height = extent.height};
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
        m_allocator = std::make_unique<VulkanAllocator>(*m_instance, *m_device);
        m_immediateCommands = std::make_unique<VulkanImmediateCommands>(*m_device);

        m_swapchain =
            std::make_unique<VulkanSwapchain>(*m_device, *m_surface, toVulkanExtent(m_window.framebufferExtent()));
        createDepthImage();

        m_meshPipeline = std::make_unique<MeshPipeline>(
            *m_device, m_swapchain->imageFormat(), m_depthImage->format(), platform::executableDirectory() / "shaders");

        m_resources = std::make_unique<RenderResources>(
            *m_device, *m_allocator, *m_immediateCommands, m_meshPipeline->materialDescriptorSetLayout());

        m_frames.reserve(MAX_FRAMES_IN_FLIGHT);
        for (std::uint32_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; ++frameIndex)
        {
            m_frames.emplace_back(*m_device,
                                  *m_allocator,
                                  m_resources->descriptorPool(),
                                  m_meshPipeline->frameDescriptorSetLayout(),
                                  sizeof(FrameUniforms));
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

    MeshHandle Renderer::createMesh(const MeshData& meshData)
    {
        return m_resources->createMesh(meshData);
    }

    TextureHandle Renderer::createTexture(const TextureData& textureData)
    {
        return m_resources->createTexture(textureData);
    }

    void Renderer::drawFrame(const FrameDescription& frameDescription)
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

        // --- 4. Update this frame's data and record its commands ---------------------
        // Safe to overwrite: the fence guarantees the GPU has finished reading
        // what this frame's uniform buffer contained.
        updateFrameUniforms(frame, frameDescription);

        // Resetting the pool recycles the memory of all its command buffers.
        frame.commandPool().reset();
        recordCommands(frame, acquiredImage.imageIndex, frameDescription);

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

    void Renderer::updateFrameUniforms(const VulkanFrame& frame, const FrameDescription& frameDescription) const
    {
        const vk::Extent2D extent = m_swapchain->extent();
        const float        aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);

        const Camera& camera = frameDescription.camera;

        const FrameUniforms frameUniforms{
            // Applied right to left: world -> view (camera) space -> clip space.
            .viewProjectionMatrix = camera.projectionMatrix(aspectRatio) * camera.viewMatrix(),
            .directionToLight = glm::vec4(glm::normalize(frameDescription.directionToLight), 0.0F),
        };

        frame.frameUniformBuffer().write(std::as_bytes(std::span(&frameUniforms, 1)));
    }

    void Renderer::recordCommands(const VulkanFrame&      frame,
                                  std::uint32_t           swapchainImageIndex,
                                  const FrameDescription& frameDescription) const
    {
        const vk::raii::CommandBuffer& commandBuffer = frame.commandBuffer();
        const vk::Image                swapchainImage = m_swapchain->image(swapchainImageIndex);
        const vk::Extent2D             extent = m_swapchain->extent();

        // "One time submit": this recording is submitted once, then discarded
        // when the pool is reset next frame.
        commandBuffer.begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        });

        // --- Prepare the images for rendering ----------------------------------------
        // Colour: Undefined -> ColorAttachmentOptimal. "Undefined" as the old
        // layout means the previous content may be discarded, which is fine
        // because the image is cleared below.
        //
        // The source stage is ColorAttachmentOutput on purpose: it is the
        // stage the submission makes wait for the image-available semaphore
        // (Renderer::drawFrame, step 5), so the transition cannot happen before
        // the image has actually been acquired.
        transitionImageLayout(commandBuffer,
                              swapchainImage,
                              ImageLayoutTransition{
                                  .oldLayout = vk::ImageLayout::eUndefined,
                                  .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                  .sourceStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  .sourceAccess = vk::AccessFlagBits2::eNone,
                                  .destinationStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  .destinationAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
                              });

        // Depth: Undefined -> DepthAttachmentOptimal. The depth buffer is
        // cleared every frame and never read afterwards, so its old content
        // can always be discarded.
        //
        // The same depth image is shared by all frames in flight. Depth is
        // read and written during the fragment test stages, so naming those
        // stages as the source makes this frame wait until the previous
        // frame has finished using the image.
        constexpr vk::PipelineStageFlags2 DEPTH_TEST_STAGES =
            vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;

        transitionImageLayout(commandBuffer,
                              m_depthImage->handle(),
                              ImageLayoutTransition{
                                  .oldLayout = vk::ImageLayout::eUndefined,
                                  .newLayout = vk::ImageLayout::eDepthAttachmentOptimal,
                                  .sourceStage = DEPTH_TEST_STAGES,
                                  .sourceAccess = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                  .destinationStage = DEPTH_TEST_STAGES,
                                  .destinationAccess = vk::AccessFlagBits2::eDepthStencilAttachmentRead
                                                       | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                                  .aspect = vk::ImageAspectFlagBits::eDepth,
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

        const vk::RenderingAttachmentInfo depthAttachment{
            .imageView = m_depthImage->view(),
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,

            // The depth buffer is only needed while drawing: letting the GPU
            // discard it afterwards saves work on some hardware.
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = vk::ClearValue(vk::ClearDepthStencilValue{.depth = CLEAR_DEPTH, .stencil = 0}),
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
            .pDepthAttachment = &depthAttachment,
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
        m_meshPipeline->bind(commandBuffer);

        // Per-frame data (set 0) is bound once and stays bound for every draw.
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_meshPipeline->layout(),
                                         MeshPipeline::FRAME_DESCRIPTOR_SET_INDEX,
                                         frame.frameDescriptorSet(),
                                         nullptr);

        for (const DrawCommand& drawCommand : frameDescription.drawCommands)
        {
            const GpuMesh&    mesh = m_resources->mesh(drawCommand.mesh);
            const GpuTexture& texture = m_resources->texture(drawCommand.texture);

            // Per-material data (set 1): the texture.
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                             m_meshPipeline->layout(),
                                             MeshPipeline::MATERIAL_DESCRIPTOR_SET_INDEX,
                                             *texture.materialDescriptorSet,
                                             nullptr);

            // Per-object data: push constants, copied into the command buffer.
            const ObjectPushConstants pushConstants{
                .modelMatrix = drawCommand.modelMatrix,
                .normalMatrix = glm::transpose(glm::inverse(drawCommand.modelMatrix)),
            };
            commandBuffer.pushConstants<ObjectPushConstants>(
                m_meshPipeline->layout(), vk::ShaderStageFlagBits::eVertex, 0, pushConstants);

            // Vertex buffer at binding 0 (the binding declared in the pipeline's
            // vertex input state), starting at byte offset 0.
            commandBuffer.bindVertexBuffers(0, mesh.vertexBuffer.handle(), vk::DeviceSize{0});
            commandBuffer.bindIndexBuffer(mesh.indexBuffer.handle(), 0, vk::IndexType::eUint32);

            // indexCount indices, 1 instance, starting at index 0, with 0
            // added to every index, starting at instance 0.
            commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        }

        commandBuffer.endRendering();

        // --- Prepare the image for presentation --------------------------------------
        // ColorAttachmentOptimal -> PresentSrc, the layout the window system
        // requires. The transition waits for the colour writes above; nothing
        // later in this command buffer depends on it (the presentation is
        // synchronised by the render-finished semaphore instead).
        transitionImageLayout(commandBuffer,
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

    void Renderer::createDepthImage()
    {
        // The depth buffer must have exactly the size of the colour images it
        // is used with. Assigning the unique_ptr destroys the previous one.
        m_depthImage = std::make_unique<VulkanImage>(*m_device,
                                                     *m_allocator,
                                                     VulkanImageDescription{
                                                         .extent = m_swapchain->extent(),
                                                         .format = m_device->findDepthFormat(),
                                                         .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                         .aspect = vk::ImageAspectFlagBits::eDepth,
                                                     });
    }

    void Renderer::recreateSwapchain()
    {
        const vk::Extent2D windowExtent = toVulkanExtent(m_window.framebufferExtent());
        if (windowExtent.width == 0 || windowExtent.height == 0)
        {
            return; // Minimised: drawFrame() will come back here once restored.
        }

        // The old swapchain images, their views and the depth buffer may
        // still be referenced by commands the GPU is executing. Waiting for
        // the device to be idle is the simplest way to make destroying them
        // safe; a resize is rare enough that the stall does not matter.
        m_device->logicalDevice().waitIdle();

        m_swapchain->recreate(windowExtent);
        createDepthImage();
    }
} // namespace mage::renderer
