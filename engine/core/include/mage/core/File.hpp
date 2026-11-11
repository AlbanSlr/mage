#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace mage
{
    // Reads a whole file into memory, byte for byte.
    // Throws std::runtime_error if the file cannot be opened or read.
    [[nodiscard]] std::vector<std::byte> readBinaryFile(const std::filesystem::path& filePath);
} // namespace mage
