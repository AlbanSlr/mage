#pragma once

#include "vulkan/VulkanApi.hpp"

#include <functional>

namespace mage::renderer
{
    class VulkanDevice;

    // Runs a few GPU commands right now and waits for them to finish.
    //
    // Some work has nothing to do with drawing a frame: copying freshly loaded
    // vertices or pixels to their final GPU buffer or image, for instance. Like
    // all GPU work it must still go through a command buffer submitted to a
    // queue. This class does the whole round trip synchronously.
    //
    // Blocking the CPU until the GPU is done is fine for loading; it must not
    // be used in the middle of a frame.
    class VulkanImmediateCommands
    {
    public:
        explicit VulkanImmediateCommands(const VulkanDevice& device);

        using RecordFunction = std::function<void(const vk::raii::CommandBuffer& commandBuffer)>;

        // Calls `recordCommands` with a command buffer ready for recording,
        // submits it, and returns once the GPU has executed it.
        void submit(const RecordFunction& recordCommands) const;

    private:
        const VulkanDevice&   m_device;
        vk::raii::CommandPool m_commandPool = nullptr;
    };
} // namespace mage::renderer
