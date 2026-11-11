# MAGE — Microburst Advanced Game Engine

A cross-platform 3D game engine written in C++20 on top of Vulkan 1.4, built to be
extended and to be read: names are explicit and every Vulkan concept is explained
where it is first used.

- [Architecture and roadmap](docs/ARCHITECTURE.md)
- [Coding style](docs/CODING_STYLE.md)

## Requirements

| Tool | Notes |
|---|---|
| clang 23 + lld | The only supported compiler, on every platform |
| CMake 3.29+ and Ninja | |
| Vulkan SDK 1.4+ | Validation layers and `slangc` |
| Windows only: Visual Studio Build Tools | Provides the Windows SDK and the C++ standard library clang uses |
| Linux only: libstdc++ 13+ and the X11/Wayland development packages | SDL3 is built from source; see `.github/workflows/ci.yml` for the package list |

## Getting the code

```sh
git clone --recurse-submodules <repository-url>
# or, in an existing clone:
git submodule update --init --recursive
```

## Building

Replace `windows-clang` with `linux-clang` on Linux.

```sh
cmake --preset windows-clang                 # configure (once)
cmake --build --preset windows-clang-debug   # build
ctest --preset windows-clang-debug           # run the unit tests
build/windows-clang/bin/Debug/mage_sandbox   # run the sandbox
```

Available configurations: `debug`, `relwithdebinfo`, `release`.

Sandbox controls: hold the right mouse button to look around, `W` `A` `S` `D` to move,
`E` / `Q` to go up / down, `Shift` to move faster, `Escape` to quit. Keys are physical
positions, so they are `Z` `Q` `S` `D` and `E` / `A` on an AZERTY keyboard.

| CMake option | Default | Effect |
|---|---|---|
| `MAGE_BUILD_TESTS` | `ON` | Build the unit tests of each module |
| `MAGE_WARNINGS_AS_ERRORS` | `OFF` | Fail the build on any warning in MAGE code |
| `MAGE_ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy while compiling MAGE code |

## Repository layout

```
engine/       engine modules (one static library each)
apps/         executables: sandbox, later the editor
shaders/      Slang shader sources, compiled to SPIR-V at build time
third_party/  dependencies, as git submodules
cmake/        build helpers
docs/         architecture and coding style
```
