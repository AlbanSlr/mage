#pragma once

#include "vulkan/VulkanApi.hpp"

#include <filesystem>

namespace mage::renderer
{
    // Loads a compiled shader file (.spv) and wraps it in a VkShaderModule,
    // the object that hands SPIR-V code to the driver.
    //
    // A shader module is only needed while pipelines are created from it; it
    // can be destroyed afterwards.
    //
    // Throws std::runtime_error if the file is missing or is not SPIR-V.
    [[nodiscard]] vk::raii::ShaderModule loadShaderModule(const vk::raii::Device&      logicalDevice,
                                                          const std::filesystem::path& filePath);
} // namespace mage::renderer
