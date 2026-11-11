#pragma once

#include "vulkan/VulkanApi.hpp"

namespace mage::renderer
{
    class VulkanDevice;

    // Everything needed to record and submit ONE frame.
    //
    // The CPU does not wait for the GPU to finish a frame before preparing the
    // next one: several frames are "in flight" at the same time. Each of them
    // needs its own set of these resources, because the GPU may still be
    // reading the previous frame's command buffer while the CPU writes the
    // next one. The renderer owns MAX_FRAMES_IN_FLIGHT of these and cycles
    // through them.
    class VulkanFrame
    {
    public:
        explicit VulkanFrame(const VulkanDevice& device);

        // A COMMAND POOL is the memory allocator command buffers are carved
        // from. One pool per frame allows the whole frame's command memory to
        // be recycled with a single, cheap pool reset.
        [[nodiscard]] const vk::raii::CommandPool& commandPool() const;

        // A COMMAND BUFFER is the list of commands (bind this, draw that)
        // recorded on the CPU and later submitted to a queue. Vulkan commands
        // are never executed immediately.
        [[nodiscard]] const vk::raii::CommandBuffer& commandBuffer() const;

        // A SEMAPHORE orders operations on the GPU side: "start B only after A
        // finished". This one is signalled when the acquired swapchain image
        // is ready to be rendered into; the frame's commands wait on it.
        [[nodiscard]] const vk::raii::Semaphore& imageAvailableSemaphore() const;

        // A FENCE lets the CPU wait for the GPU. This one is signalled when
        // the GPU finished executing this frame's commands, which means this
        // object's resources can be reused for a new frame.
        [[nodiscard]] const vk::raii::Fence& inFlightFence() const;

    private:
        // Declared before the command buffer: the buffer is allocated from the
        // pool and must be destroyed first (members die in reverse order).
        vk::raii::CommandPool   m_commandPool = nullptr;
        vk::raii::CommandBuffer m_commandBuffer = nullptr;

        vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
        vk::raii::Fence     m_inFlightFence = nullptr;
    };
} // namespace mage::renderer
