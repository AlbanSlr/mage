#include "vulkan/VulkanAllocator.hpp"

#include "vulkan/VulkanApi.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"

#include <mage/core/Log.hpp>

#include <vk_mem_alloc.h>

#include <format>
#include <stdexcept>

namespace mage::renderer
{
    VulkanAllocator::VulkanAllocator(const VulkanInstance& instance, const VulkanDevice& device)
    {
        // MAGE does not link against the Vulkan library, so VMA cannot find
        // the Vulkan functions by itself. Given these two entry points, it
        // looks up every other function it needs (VMA_DYNAMIC_VULKAN_FUNCTIONS).
        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetInstanceProcAddr = instance.handle().getDispatcher()->vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = device.logicalDevice().getDispatcher()->vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.instance = *instance.handle();
        allocatorCreateInfo.physicalDevice = *device.physicalDevice();
        allocatorCreateInfo.device = *device.logicalDevice();
        allocatorCreateInfo.vulkanApiVersion = vk::ApiVersion14;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        const auto result = static_cast<vk::Result>(vmaCreateAllocator(&allocatorCreateInfo, &m_allocator));
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error(
                std::format("Failed to create the GPU memory allocator: {}", vk::to_string(result)));
        }

        log::info("Created GPU memory allocator");
    }

    VulkanAllocator::~VulkanAllocator()
    {
        // Every buffer and image must have been destroyed before this point;
        // VMA asserts if an allocation is still alive.
        vmaDestroyAllocator(m_allocator);
    }

    VmaAllocator VulkanAllocator::handle() const
    {
        return m_allocator;
    }
} // namespace mage::renderer
