#pragma once

#include <cstdint>
#include <optional>

// Parses the optional `--max-frames=<count>` command line argument, which
// makes an application exit by itself after rendering that many frames. It is
// used to run the applications unattended (automated smoke tests).
//
// Returns nothing if the argument is absent or invalid.
[[nodiscard]] std::optional<std::uint64_t> parseMaxFrameCount(int argumentCount, const char* const* arguments);
