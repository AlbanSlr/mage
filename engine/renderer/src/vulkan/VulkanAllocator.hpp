#pragma once

#include "vulkan/VulkanApi.hpp"

#include <vk_mem_alloc.h>

namespace mage::renderer
{
    class VulkanDevice;
    class VulkanInstance;

    // GPU memory allocation, delegated to the Vulkan Memory Allocator (VMA).
    //
    // In Vulkan, creating a buffer or an image does not give it any memory.
    // The application must separately allocate device memory (choosing among
    // the memory types the GPU exposes) and bind it to the object. Drivers
    // also limit the number of allocations, so real programs allocate large
    // blocks and place many resources inside each one.
    //
    // VMA is the library, written by AMD and used by most Vulkan engines, that
    // does exactly this. MAGE keeps the rest of Vulkan explicit and lets VMA
    // handle this one bookkeeping-heavy job.
    //
    // VMA is a C library: unlike vk::raii objects, what it creates must be
    // released by hand. VulkanBuffer and VulkanImage wrap that in RAII classes.
    class VulkanAllocator
    {
    public:
        VulkanAllocator(const VulkanInstance& instance, const VulkanDevice& device);
        ~VulkanAllocator();

        VulkanAllocator(const VulkanAllocator&) = delete;
        VulkanAllocator& operator=(const VulkanAllocator&) = delete;
        VulkanAllocator(VulkanAllocator&&) = delete;
        VulkanAllocator& operator=(VulkanAllocator&&) = delete;

        [[nodiscard]] VmaAllocator handle() const;

    private:
        VmaAllocator m_allocator = nullptr;
    };
} // namespace mage::renderer
