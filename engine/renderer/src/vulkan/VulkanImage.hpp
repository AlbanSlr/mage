#pragma once

#include "vulkan/VulkanApi.hpp"

#include <vk_mem_alloc.h>

namespace mage::renderer
{
    class VulkanAllocator;
    class VulkanDevice;

    struct VulkanImageDescription
    {
        vk::Extent2D extent;
        vk::Format   format = vk::Format::eUndefined;

        // What the image will be used as: sampled by shaders, rendered into,
        // destination of a copy... Must be declared at creation.
        vk::ImageUsageFlags usage;

        // Which part of the pixel data views of this image give access to:
        // colour for textures, depth for depth buffers.
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;

        // Optional second format the image can be viewed with. When set, the
        // image gets an additional view (VulkanImage::alternateView) that
        // reinterprets the same bytes with this format. The two formats must
        // have the same size per pixel.
        vk::Format alternateViewFormat = vk::Format::eUndefined;
    };

    // A 2D IMAGE created by the engine (as opposed to swapchain images, which
    // belong to the window system): a texture or a depth buffer. It owns the
    // image, the memory backing it, and one view of the whole image.
    class VulkanImage
    {
    public:
        VulkanImage(const VulkanDevice&           device,
                    const VulkanAllocator&        allocator,
                    const VulkanImageDescription& description);
        ~VulkanImage();

        // Owns a GPU resource: movable, not copyable.
        VulkanImage(const VulkanImage&) = delete;
        VulkanImage& operator=(const VulkanImage&) = delete;
        VulkanImage(VulkanImage&& other) noexcept;
        VulkanImage& operator=(VulkanImage&& other) noexcept;

        [[nodiscard]] vk::Image handle() const;
        [[nodiscard]] vk::ImageView view() const;

        // Only valid if the image was created with an alternateViewFormat.
        [[nodiscard]] vk::ImageView alternateView() const;
        [[nodiscard]] vk::Format format() const;
        [[nodiscard]] vk::Extent2D extent() const;

    private:
        void destroy();

        VmaAllocator  m_allocator = nullptr;
        VkImage       m_image = VK_NULL_HANDLE;
        VmaAllocation m_allocation = nullptr;

        vk::raii::ImageView m_view = nullptr;
        vk::raii::ImageView m_alternateView = nullptr;

        vk::Format   m_format = vk::Format::eUndefined;
        vk::Extent2D m_extent;
    };
} // namespace mage::renderer
