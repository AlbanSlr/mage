#include "vulkan/VulkanImage.hpp"

#include "vulkan/VulkanAllocator.hpp"
#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"

#include <vk_mem_alloc.h>

#include <format>
#include <stdexcept>
#include <utility>

namespace mage::renderer
{
    VulkanImage::VulkanImage(const VulkanDevice&           device,
                             const VulkanAllocator&        allocator,
                             const VulkanImageDescription& description)
        : m_allocator(allocator.handle())
        , m_format(description.format)
        , m_extent(description.extent)
    {
        const vk::ImageCreateInfo imageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = description.format,

            // A 2D image is a 3D image that is one pixel deep.
            .extent = vk::Extent3D{.width = description.extent.width, .height = description.extent.height, .depth = 1},

            // Mip levels are progressively smaller copies of the image, used
            // when it is seen from far away. Only the full-size level for now.
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,

            // Optimal tiling lets the driver arrange the pixels in memory in
            // whatever way is fastest for the GPU. The alternative (linear,
            // row by row) is only needed to access pixels directly from the CPU.
            .tiling = vk::ImageTiling::eOptimal,
            .usage = description.usage,
            .sharingMode = vk::SharingMode::eExclusive,

            // A new image contains nothing, so its layout is "undefined". It
            // is moved to a usable layout before its first use.
            .initialLayout = vk::ImageLayout::eUndefined,
        };

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        const VkImageCreateInfo& nativeImageCreateInfo = imageCreateInfo;

        const auto result = static_cast<vk::Result>(vmaCreateImage(
            m_allocator, &nativeImageCreateInfo, &allocationCreateInfo, &m_image, &m_allocation, nullptr));
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(std::format("Failed to create a {}x{} image: {}",
                                                 description.extent.width,
                                                 description.extent.height,
                                                 vk::to_string(result)));
        }

        // Images are always accessed through a view (see VulkanSwapchain.hpp).
        const vk::ImageViewCreateInfo viewCreateInfo{
            .image = vk::Image(m_image),
            .viewType = vk::ImageViewType::e2D,
            .format = description.format,
            .subresourceRange =
                vk::ImageSubresourceRange{
                    .aspectMask = description.aspect,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        try
        {
            m_view = device.logicalDevice().createImageView(viewCreateInfo);
        }
        catch (...)
        {
            // The destructor does not run when a constructor throws: release
            // the image by hand before letting the exception continue.
            destroy();
            throw;
        }
    }

    VulkanImage::~VulkanImage()
    {
        destroy();
    }

    VulkanImage::VulkanImage(VulkanImage&& other) noexcept
        : m_allocator(other.m_allocator)
        , m_image(std::exchange(other.m_image, VK_NULL_HANDLE))
        , m_allocation(std::exchange(other.m_allocation, nullptr))
        , m_view(std::move(other.m_view))
        , m_format(other.m_format)
        , m_extent(other.m_extent)
    {
    }

    VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            m_allocator = other.m_allocator;
            m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
            m_allocation = std::exchange(other.m_allocation, nullptr);
            m_view = std::move(other.m_view);
            m_format = other.m_format;
            m_extent = other.m_extent;
        }
        return *this;
    }

    void VulkanImage::destroy()
    {
        // The view refers to the image and must be destroyed first. Member
        // destructors would run too late (after this function), so the view
        // is released explicitly.
        m_view.clear();

        // A moved-from image owns nothing.
        if (m_image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_allocator, m_image, m_allocation);
            m_image = VK_NULL_HANDLE;
            m_allocation = nullptr;
        }
    }

    vk::Image VulkanImage::handle() const
    {
        return vk::Image(m_image);
    }

    vk::ImageView VulkanImage::view() const
    {
        return *m_view;
    }

    vk::Format VulkanImage::format() const
    {
        return m_format;
    }

    vk::Extent2D VulkanImage::extent() const
    {
        return m_extent;
    }
} // namespace mage::renderer
