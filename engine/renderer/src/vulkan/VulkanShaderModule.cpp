#include "vulkan/VulkanShaderModule.hpp"

#include "vulkan/VulkanApi.hpp"

#include <mage/core/File.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <vector>

namespace mage::renderer
{
    vk::raii::ShaderModule loadShaderModule(const vk::raii::Device&      logicalDevice,
                                            const std::filesystem::path& filePath)
    {
        const std::vector<std::byte> fileContent = readBinaryFile(filePath);

        // SPIR-V is a stream of 32-bit words, and Vulkan wants it as such.
        if (fileContent.empty() || fileContent.size() % sizeof(std::uint32_t) != 0)
        {
            throw std::runtime_error(std::format("'{}' is not a valid SPIR-V file", filePath.string()));
        }

        // Copy into uint32_t storage so the data is correctly aligned.
        std::vector<std::uint32_t> spirvWords(fileContent.size() / sizeof(std::uint32_t));
        std::memcpy(spirvWords.data(), fileContent.data(), fileContent.size());

        const vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
            .codeSize = fileContent.size(), // In bytes, not in words.
            .pCode = spirvWords.data(),
        };

        return logicalDevice.createShaderModule(shaderModuleCreateInfo);
    }
} // namespace mage::renderer
