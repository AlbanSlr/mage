#include <mage/core/File.hpp>

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

namespace mage
{
    std::vector<std::byte> readBinaryFile(const std::filesystem::path& filePath)
    {
        // std::ios::ate opens the file with the cursor at the end, so the
        // cursor position directly gives the file size.
        // NOLINT: the standard library defines these flags as plain integers on
        // some platforms, which clang-tidy mistakes for arithmetic on signed values.
        std::ifstream file(filePath, std::ios::binary | std::ios::ate); // NOLINT(bugprone-signed-bitwise)
        if (!file.is_open())
        {
            throw std::runtime_error(std::format("Failed to open file '{}'", filePath.string()));
        }

        const std::streamsize fileSize = file.tellg();
        if (fileSize < 0)
        {
            throw std::runtime_error(std::format("Failed to get the size of file '{}'", filePath.string()));
        }

        std::vector<std::byte> fileContent(static_cast<std::size_t>(fileSize));

        file.seekg(0, std::ios::beg);
        // The stream API works with char*; reading into std::byte storage
        // through it is one of the few casts the language explicitly allows.
        file.read(reinterpret_cast<char*>(fileContent.data()), fileSize);
        if (!file)
        {
            throw std::runtime_error(std::format("Failed to read file '{}'", filePath.string()));
        }

        return fileContent;
    }
} // namespace mage
