#pragma once

#include <filesystem>

namespace mage::platform
{
    // Directory containing the running executable. Files shipped next to the
    // executable (compiled shaders, later assets) are located relative to it,
    // so the program works whatever the current working directory is.
    //
    // Throws std::runtime_error if the directory cannot be determined.
    [[nodiscard]] std::filesystem::path executableDirectory();
} // namespace mage::platform
