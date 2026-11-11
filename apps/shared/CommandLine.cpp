#include <mage/core/Log.hpp>

#include <shared/CommandLine.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

std::optional<std::uint64_t> parseMaxFrameCount(int argumentCount, const char* const* arguments)
{
    constexpr std::string_view OPTION_PREFIX = "--max-frames=";

    // Argument 0 is the name of the program itself.
    for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
    {
        const std::string_view argument = arguments[argumentIndex];
        if (!argument.starts_with(OPTION_PREFIX))
        {
            continue;
        }

        const std::string_view value = argument.substr(OPTION_PREFIX.size());
        std::uint64_t          maxFrameCount = 0;

        // from_chars reports both conversion errors and where it stopped: the
        // value is only valid if the whole text was consumed.
        const std::from_chars_result parseResult =
            std::from_chars(value.data(), value.data() + value.size(), maxFrameCount);
        if (parseResult.ec == std::errc{} && parseResult.ptr == value.data() + value.size())
        {
            return maxFrameCount;
        }

        mage::log::warning("Ignoring invalid argument '{}'", argument);
    }

    return std::nullopt;
}
