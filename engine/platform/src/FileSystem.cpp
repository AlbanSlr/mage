#include <mage/platform/FileSystem.hpp>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_filesystem.h>

#include <filesystem>
#include <format>
#include <stdexcept>

namespace mage::platform
{
    std::filesystem::path executableDirectory()
    {
        // SDL caches this string internally: it must not be freed.
        const char* basePath = SDL_GetBasePath();
        if (basePath == nullptr)
        {
            throw std::runtime_error(std::format("Failed to locate the executable directory: {}", SDL_GetError()));
        }

        return {basePath};
    }
} // namespace mage::platform
