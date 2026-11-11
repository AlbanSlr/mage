# MAGE coding style

Formatting is enforced by `.clang-format`, naming and static analysis by `.clang-tidy`.
This document covers the rules and the conventions a tool cannot check.

## Naming

| Element | Convention | Example |
|---|---|---|
| Types (class, struct, enum, alias, concept) | `PascalCase` | `RenderDevice`, `SinkHandle` |
| Enum values | `PascalCase` | `Level::Warning` |
| Functions and methods | `camelCase` | `createSwapchain()` |
| Local variables and parameters | `camelCase` | `imageCount` |
| Private / protected members | `m_` + `camelCase` | `m_logicalDevice` |
| Public struct fields | `camelCase` | `extent.width` |
| Static members | `s_` + `camelCase` | `s_instanceCount` |
| Global variables (avoid) | `g_` + `camelCase` | `g_allocator` |
| Constants (`constexpr`, global or static `const`) | `UPPER_CASE` | `MAX_FRAMES_IN_FLIGHT` |
| Macros | `MAGE_` + `UPPER_CASE` | `MAGE_ASSERT` |
| Namespaces | `lower_case` | `mage`, `mage::log` |
| Files | `PascalCase`, named after the main type | `RenderDevice.hpp` / `.cpp` |

**Names are explicit.** MAGE is meant to be read to learn Vulkan, so prefer the full
word to an abbreviation: `swapchainImageIndex`, not `imgIdx`; `commandBuffer`, not `cmd`.
Keep the Vulkan vocabulary as-is (`swapchain`, `descriptorSet`, `imageView`) so names
can be looked up directly in the specification.

## Files and includes

- Headers use `#pragma once` and the `.hpp` extension; sources use `.cpp`.
- Public headers of a module live in `engine/<module>/include/mage/<module>/` and are
  included as `<mage/<module>/File.hpp>`. Private headers live in `src/` and are
  included with quotes.
- Include order (sorted automatically): local `"..."` headers, `<mage/...>`,
  third-party, standard library.
- Include what you use; do not rely on transitive includes.
- Third-party types stay out of public headers when a small wrapper is enough
  (for example, spdlog only appears in `Log.cpp`).

## Comments

- Comment the **why**, not the what. The code already says what it does.
- Every Vulkan concept gets an explanation **at its first use** in the engine:
  what the object is, why Vulkan needs it, and why it is configured this way.
- Each public type and non-obvious function has a short comment above it.
- Use `//` comments. No commented-out code.

## Language rules

- C++20, no compiler extensions, no C++20 modules.
- Everything lives in the `mage` namespace; helpers that are not part of the public
  API go in `mage::detail` or an anonymous namespace.
- **Ownership is RAII.** Vulkan objects are `vk::raii` types; no manual `destroy` calls,
  no owning raw pointers. Members are declared in creation order so they are
  destroyed in reverse order.
- Mark single-argument constructors `explicit`, overriding functions `override`,
  and value-returning queries `[[nodiscard]]`.
- `const` by default for locals and member functions.
- Always use braces, even for a single statement.
- Use `static_cast` and friends, never C-style casts.
- Prefer `enum class` and fixed-width integer types (`std::uint32_t`) for data
  that crosses an API boundary.

## Errors

| Situation | Mechanism |
|---|---|
| Runtime failure during setup or loading (no suitable GPU, missing file) | throw an exception |
| Programmer error (broken invariant, invalid argument) | `MAGE_ASSERT(condition, "message {}", value)` |
| Expected per-frame outcome (swapchain out of date) | return value, handled by the caller |

Exceptions are not used in per-frame hot paths. Assertions are compiled out of
Release builds, so their condition must not have side effects.

## Logging

Use `mage::log::info(...)` and its siblings with `std::format` syntax. Never use
`std::cout` or `printf` in engine code.

## Tooling

```sh
# Format every source file
git ls-files '*.hpp' '*.cpp' | xargs clang-format -i

# Build with clang-tidy and warnings as errors (what CI does)
cmake --preset windows-clang -DMAGE_ENABLE_CLANG_TIDY=ON -DMAGE_WARNINGS_AS_ERRORS=ON
```
