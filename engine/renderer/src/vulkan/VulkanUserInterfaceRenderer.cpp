#include "vulkan/VulkanUserInterfaceRenderer.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"

#include <mage/core/Assert.hpp>
#include <mage/core/Log.hpp>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace mage::renderer
{
    namespace
    {
        // Number of textures the interface can display at the same time
        // (its font atlas plus images such as the scene viewport).
        constexpr std::uint32_t MAX_USER_INTERFACE_TEXTURE_COUNT = 64;

        // Dear ImGui calls this with the result of its own Vulkan calls.
        void checkVulkanResult(VkResult result)
        {
            if (result != VK_SUCCESS)
            {
                log::error("[Dear ImGui] Vulkan call failed: {}", vk::to_string(static_cast<vk::Result>(result)));
            }
        }

        // MAGE does not link against the Vulkan library, so the backend
        // cannot call Vulkan functions by name. It calls this instead to look
        // each one up, through the instance the engine created.
        // NOLINTNEXTLINE(misc-const-correctness): the signature is imposed by Dear ImGui.
        PFN_vkVoidFunction loadVulkanFunction(const char* functionName, void* userData)
        {
            const auto* instance = static_cast<const vk::raii::Instance*>(userData);
            return instance->getDispatcher()->vkGetInstanceProcAddr(static_cast<VkInstance>(**instance), functionName);
        }
    } // namespace

    VulkanUserInterfaceRenderer::VulkanUserInterfaceRenderer(const VulkanInstance& instance,
                                                             const VulkanDevice&   device,
                                                             vk::Format            colorAttachmentFormat,
                                                             std::uint32_t         swapchainImageCount)
        : m_colorAttachmentFormat(colorAttachmentFormat)
    {
        MAGE_ASSERT(ImGui::GetCurrentContext() != nullptr,
                    "A Dear ImGui context must exist before the renderer is created (create ui::UserInterface first)");

        // The loader only reads through the pointer; the cast is needed
        // because the callback's user data parameter is a plain void*.
        void* loaderUserData = const_cast<vk::raii::Instance*>(&instance.handle());
        if (!ImGui_ImplVulkan_LoadFunctions(vk::ApiVersion14, &loadVulkanFunction, loaderUserData))
        {
            throw std::runtime_error("Dear ImGui could not load the Vulkan functions it needs");
        }

        // Tells the backend which attachment format its pipeline will draw
        // into. This is the dynamic rendering equivalent of a render pass.
        const vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &m_colorAttachmentFormat,
        };

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = vk::ApiVersion14;
        initInfo.Instance = *instance.handle();
        initInfo.PhysicalDevice = *device.physicalDevice();
        initInfo.Device = *device.logicalDevice();
        initInfo.QueueFamily = device.graphicsQueueFamilyIndex();
        initInfo.Queue = *device.graphicsQueue();

        // Let the backend create its own descriptor pool for its textures.
        initInfo.DescriptorPoolSize = MAX_USER_INTERFACE_TEXTURE_COUNT;

        // Used by the backend to size its per-frame vertex buffers.
        initInfo.MinImageCount = std::max(2U, swapchainImageCount);
        initInfo.ImageCount = std::max(2U, swapchainImageCount);

        initInfo.UseDynamicRendering = true;
        initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;
        initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.CheckVkResultFn = &checkVulkanResult;

        if (!ImGui_ImplVulkan_Init(&initInfo))
        {
            throw std::runtime_error("Failed to initialise the Dear ImGui Vulkan backend");
        }

        log::info("Initialised the user interface renderer");
    }

    VulkanUserInterfaceRenderer::~VulkanUserInterfaceRenderer()
    {
        ImGui_ImplVulkan_Shutdown();
    }

    void VulkanUserInterfaceRenderer::beginFrame() const
    {
        ImGui_ImplVulkan_NewFrame();
    }

    void VulkanUserInterfaceRenderer::render(const vk::raii::CommandBuffer& commandBuffer) const
    {
        // The draw data is the list of triangles produced by ImGui::Render().
        // It is null until the first interface frame has been completed.
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData == nullptr)
        {
            return;
        }

        ImGui_ImplVulkan_RenderDrawData(drawData, *commandBuffer);
    }

    std::uint64_t VulkanUserInterfaceRenderer::registerTexture(vk::ImageView imageView) const
    {
        // The backend creates a descriptor set (combined image sampler) for
        // the image; that descriptor set IS the texture identifier Dear ImGui
        // works with. A descriptor set handle is a pointer-sized value, which
        // is stored in the 64-bit integer Dear ImGui uses for identifiers.
        const VkDescriptorSet descriptorSet =
            ImGui_ImplVulkan_AddTexture(imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return std::bit_cast<std::uint64_t>(descriptorSet);
    }

    void VulkanUserInterfaceRenderer::unregisterTexture(std::uint64_t textureId) const
    {
        ImGui_ImplVulkan_RemoveTexture(std::bit_cast<VkDescriptorSet>(textureId));
    }
} // namespace mage::renderer
