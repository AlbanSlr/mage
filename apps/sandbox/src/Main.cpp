#include <mage/core/Log.hpp>
#include <mage/platform/Window.hpp>
#include <mage/renderer/Renderer.hpp>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <optional>
#include <string_view>

namespace
{
    // Parses the optional `--max-frames=<count>` argument, which makes the
    // sandbox exit by itself after rendering that many frames. It is used to
    // run the sandbox unattended (automated smoke tests).
    std::optional<std::uint64_t> parseMaxFrameCount(int argumentCount, const char* const* arguments)
    {
        constexpr std::string_view OPTION_PREFIX = "--max-frames=";

        for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
        {
            const std::string_view argument = arguments[argumentIndex];
            if (!argument.starts_with(OPTION_PREFIX))
            {
                continue;
            }

            const std::string_view value = argument.substr(OPTION_PREFIX.size());
            std::uint64_t          maxFrameCount = 0;
            const auto parseResult = std::from_chars(value.data(), value.data() + value.size(), maxFrameCount);
            if (parseResult.ec == std::errc{} && parseResult.ptr == value.data() + value.size())
            {
                return maxFrameCount;
            }

            mage::log::warning("Ignoring invalid argument '{}'", argument);
        }

        return std::nullopt;
    }
} // namespace

// The sandbox is the playground application used to exercise engine features
// while they are being developed.
// NOLINT: the C++ standard fixes the signature of main(); its arguments cannot be const.
int main(int argumentCount, char** arguments) // NOLINT(misc-const-correctness)
{
    // Setup failures are reported with exceptions (see docs/CODING_STYLE.md),
    // so the entry point is the single place that catches and reports them.
    try
    {
        const std::optional<std::uint64_t> maxFrameCount = parseMaxFrameCount(argumentCount, arguments);

        mage::log::info("MAGE sandbox starting");

        // Declaration order matters: the renderer uses the window, so it is
        // declared after it and therefore destroyed before it.
        mage::platform::Window window(mage::platform::WindowDescription{
            .title = "MAGE Sandbox",
        });

        mage::renderer::Renderer renderer(window,
                                          mage::renderer::RendererDescription{
                                              .applicationName = "MAGE Sandbox",
                                          });

        // The main loop: one iteration per frame.
        std::uint64_t renderedFrameCount = 0;
        while (!window.isCloseRequested())
        {
            window.pollEvents();
            renderer.drawFrame();

            ++renderedFrameCount;
            if (maxFrameCount.has_value() && renderedFrameCount >= *maxFrameCount)
            {
                break;
            }
        }

        mage::log::info("MAGE sandbox shutting down after {} frames", renderedFrameCount);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        mage::log::critical("Unhandled exception: {}", exception.what());
        return EXIT_FAILURE;
    }
}
