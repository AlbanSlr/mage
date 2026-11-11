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
| Linux only: libstdc++ 13+ | |

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

| CMake option | Default | Effect |
|---|---|---|
| `MAGE_BUILD_TESTS` | `ON` | Build the unit tests of each module |
| `MAGE_WARNINGS_AS_ERRORS` | `OFF` | Fail the build on any warning in MAGE code |
| `MAGE_ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy while compiling MAGE code |

## Repository layout

```
engine/       engine modules (one static library each)
apps/         executables: sandbox, later the editor
shaders/      Slang shader sources (from the GPU resources milestone)
third_party/  dependencies, as git submodules
cmake/        build helpers
docs/         architecture and coding style
```
