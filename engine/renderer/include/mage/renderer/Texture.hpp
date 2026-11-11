#pragma once

#include <cstdint>
#include <vector>

namespace mage::renderer
{
    // An image on the CPU side, ready to be uploaded with Renderer::createTexture().
    struct TextureData
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        // width * height pixels, row by row starting from the top-left corner.
        // Each pixel is 4 bytes: red, green, blue, alpha. Colours are sRGB
        // encoded, like the content of ordinary image files.
        std::vector<std::uint8_t> rgbaPixels;
    };

    // Identifies a texture that lives on the GPU. Returned by Renderer::createTexture().
    enum class TextureHandle : std::uint32_t
    {
    };
} // namespace mage::renderer
