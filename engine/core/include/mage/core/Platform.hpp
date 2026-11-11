#pragma once

// Compile-time platform detection.
//
// Exactly one MAGE_PLATFORM_* macro is defined. Platform-specific code should
// live in the `platform` module; other modules use these macros only when
// there is no reasonable alternative.

#if defined(_WIN32)
    #define MAGE_PLATFORM_WINDOWS
#elif defined(__linux__)
    #define MAGE_PLATFORM_LINUX
#else
    #error "MAGE: unsupported platform (supported: Windows, Linux)."
#endif

#if !defined(__clang__)
    #error "MAGE: unsupported compiler. MAGE is built with clang on every platform."
#endif

// Stops execution in an attached debugger at the exact line where it is
// expanded. Without a debugger the process is terminated by the trap.
#define MAGE_DEBUG_BREAK() __builtin_debugtrap()
