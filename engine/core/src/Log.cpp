#include <mage/core/Log.hpp>

#include <spdlog/logger.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mage::log
{
    namespace
    {
        spdlog::level::level_enum toSpdlogLevel(Level level)
        {
            switch (level)
            {
                case Level::Trace:
                    return spdlog::level::trace;
                case Level::Debug:
                    return spdlog::level::debug;
                case Level::Info:
                    return spdlog::level::info;
                case Level::Warning:
                    return spdlog::level::warn;
                case Level::Error:
                    return spdlog::level::err;
                case Level::Critical:
                    return spdlog::level::critical;
            }
            return spdlog::level::info;
        }

        Level fromSpdlogLevel(spdlog::level::level_enum level)
        {
            switch (level)
            {
                case spdlog::level::trace:
                    return Level::Trace;
                case spdlog::level::debug:
                    return Level::Debug;
                case spdlog::level::info:
                    return Level::Info;
                case spdlog::level::warn:
                    return Level::Warning;
                case spdlog::level::err:
                    return Level::Error;
                default:
                    return Level::Critical;
            }
        }

        // Everything the logger owns. Created on first use, so logging works
        // from any point of the program without an explicit initialisation call.
        struct LoggerState
        {
            spdlog::logger                                      logger;
            std::mutex                                          sinksMutex;
            std::unordered_map<std::uint32_t, spdlog::sink_ptr> userSinks;
            std::uint32_t                                       nextSinkId = 1;

            LoggerState()
                : logger("MAGE", std::make_shared<spdlog::sinks::stdout_color_sink_mt>())
            {
                // Example output: [14:03:27.512] [info] Created swapchain
                // %^ and %$ delimit the part coloured according to the level.
                logger.set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
                logger.set_level(spdlog::level::trace);

                // Errors are flushed immediately so they are never lost if the
                // program crashes right after reporting them.
                logger.flush_on(spdlog::level::err);
            }
        };

        LoggerState& loggerState()
        {
            static LoggerState state;
            return state;
        }
    } // namespace

    void setMinimumLevel(Level minimumLevel)
    {
        loggerState().logger.set_level(toSpdlogLevel(minimumLevel));
    }

    bool isEnabled(Level level)
    {
        return loggerState().logger.should_log(toSpdlogLevel(level));
    }

    void flush()
    {
        loggerState().logger.flush();
    }

    SinkHandle addSink(SinkCallback callback)
    {
        LoggerState& state = loggerState();

        auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [userCallback = std::move(callback)](const spdlog::details::log_msg& logMessage)
            {
                const std::string_view message(logMessage.payload.data(), logMessage.payload.size());
                userCallback(fromSpdlogLevel(logMessage.level), message);
            });

        const std::scoped_lock lock(state.sinksMutex);
        const std::uint32_t    sinkId = state.nextSinkId++;
        state.userSinks.emplace(sinkId, sink);
        state.logger.sinks().push_back(std::move(sink));
        return SinkHandle{sinkId};
    }

    void removeSink(SinkHandle sinkHandle)
    {
        LoggerState& state = loggerState();

        const std::scoped_lock lock(state.sinksMutex);
        const auto             entry = state.userSinks.find(static_cast<std::uint32_t>(sinkHandle));
        if (entry == state.userSinks.end())
        {
            return;
        }

        std::erase(state.logger.sinks(), entry->second);
        state.userSinks.erase(entry);
    }

    namespace detail
    {
        void writeFormatted(Level level, std::string_view format, std::format_args arguments)
        {
            LoggerState& state = loggerState();

            // Check the level first: a discarded message costs no formatting.
            const spdlog::level::level_enum spdlogLevel = toSpdlogLevel(level);
            if (!state.logger.should_log(spdlogLevel))
            {
                return;
            }

            const std::string message = std::vformat(format, arguments);

            // The sink list is only modified under this mutex (addSink/removeSink),
            // so holding it here keeps the list stable while the message is written.
            const std::scoped_lock lock(state.sinksMutex);
            state.logger.log(spdlogLevel, std::string_view(message));
        }
    } // namespace detail
} // namespace mage::log
