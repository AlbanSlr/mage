#pragma once

#include <mage/core/Platform.hpp>

#include <cstdlib>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

// Assertions document and check assumptions that can only be false because of
// a programming error (a bug in MAGE or in the code calling it).
//
//     MAGE_ASSERT(imageIndex < m_swapchainImages.size(),
//                 "Image index {} is out of range", imageIndex);
//
// They are NOT for errors that can legitimately happen at runtime (missing
// file, unsupported GPU feature, device lost): those throw an exception.
//
// Assertions are active when MAGE_ENABLE_ASSERTS is defined (Debug and
// RelWithDebInfo builds) and compile to nothing in Release builds, so the
// condition must never have side effects the program relies on.

namespace mage::detail
{
    // Logs the failed assertion as a Critical message and flushes the log.
    void reportAssertionFailure(std::string_view            expression,
                                const std::string&          message,
                                const std::source_location& location);
} // namespace mage::detail

#if defined(MAGE_ENABLE_ASSERTS)

    // A message is mandatory: it should say what was expected, so that the
    // log alone is enough to understand the failure.
    #define MAGE_ASSERT(condition, ...)                                                         \
        do                                                                                      \
        {                                                                                       \
            if (!(condition)) [[unlikely]]                                                      \
            {                                                                                   \
                ::mage::detail::reportAssertionFailure(                                         \
                    #condition, ::std::format(__VA_ARGS__), ::std::source_location::current()); \
                MAGE_DEBUG_BREAK();                                                             \
                ::std::abort();                                                                 \
            }                                                                                   \
        } while (false)

#else

    // The condition is not evaluated, but `sizeof` still makes the compiler
    // parse it, so an assertion cannot silently stop compiling in Release.
    #define MAGE_ASSERT(condition, ...) static_cast<void>(sizeof(static_cast<bool>(condition)))

#endif
