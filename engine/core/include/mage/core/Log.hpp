#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <string_view>

// Engine-wide logging.
//
// Usage (std::format syntax):
//
//     mage::log::info("Created swapchain with {} images", imageCount);
//     mage::log::error("Failed to open '{}'", filePath);
//
// The logging backend (spdlog) is an implementation detail of Log.cpp and
// never appears in a public header, so it can be replaced without touching
// the rest of the engine.
namespace mage::log
{
    enum class Level : std::uint8_t
    {
        Trace,    // Very detailed flow information, usually disabled.
        Debug,    // Information useful while developing a feature.
        Info,     // Normal, noteworthy events (startup, resource creation).
        Warning,  // Something unexpected that the engine recovered from.
        Error,    // An operation failed; the engine keeps running.
        Critical, // The engine cannot continue.
    };

    // Messages below `minimumLevel` are discarded before being formatted.
    void setMinimumLevel(Level minimumLevel);

    [[nodiscard]] bool isEnabled(Level level);

    // Forces buffered messages to be written out. Called automatically for
    // Error and Critical messages.
    void flush();

    // --- Sinks ---------------------------------------------------------------
    // A sink receives every message that passes the minimum level, in addition
    // to the console. The editor uses one to feed its console panel.

    using SinkCallback = std::function<void(Level level, std::string_view message)>;

    enum class SinkHandle : std::uint32_t
    {
    };

    // The callback may be invoked from any thread that logs.
    [[nodiscard]] SinkHandle addSink(SinkCallback callback);

    void removeSink(SinkHandle sinkHandle);

    namespace detail
    {
        // Type-erased entry point: keeps the formatting and backend code in
        // Log.cpp instead of instantiating it at every call site.
        void writeFormatted(Level level, std::string_view format, std::format_args arguments);
    } // namespace detail

    // --- Logging functions ---------------------------------------------------
    // `std::format_string` makes the compiler check the format string against
    // the argument types, so a mismatch is a compile error, not a runtime one.

    template <typename... Arguments>
    void trace(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Trace, format.get(), std::make_format_args(arguments...));
    }

    template <typename... Arguments>
    void debug(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Debug, format.get(), std::make_format_args(arguments...));
    }

    template <typename... Arguments>
    void info(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Info, format.get(), std::make_format_args(arguments...));
    }

    template <typename... Arguments>
    void warning(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Warning, format.get(), std::make_format_args(arguments...));
    }

    template <typename... Arguments>
    void error(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Error, format.get(), std::make_format_args(arguments...));
    }

    template <typename... Arguments>
    void critical(std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        detail::writeFormatted(Level::Critical, format.get(), std::make_format_args(arguments...));
    }
} // namespace mage::log
