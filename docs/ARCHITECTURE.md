# MAGE architecture

MAGE is split into modules. Each module is a static library with its own public
headers, private sources and tests, declared with `mage_add_module()`
(see `cmake/MageModule.cmake`).

## Dependency rule

A module may only depend on modules **below** it in this table. Dependencies are
listed explicitly in each module's `CMakeLists.txt`, so a forbidden include fails
to compile instead of creating a hidden cycle.

| Module | Responsibility | Depends on | Status |
|---|---|---|---|
| `apps/editor`, `apps/sandbox` | Executables | runtime (sandbox: renderer, for now) | sandbox exists |
| `runtime` | Application loop, layer stack | all modules below | planned |
| `scene` | Components, transform hierarchy, serialization | core, ecs, assets | planned |
| `assets` | Importers, asset registry and cache | core, renderer | planned |
| `renderer` | Vulkan device, swapchain, resources, pipelines | core, platform | exists (textured, lit meshes) |
| `ecs` | Custom entity-component-system | core | planned |
| `platform` | SDL3 window, input, filesystem | core | exists (window, input, Vulkan surface) |
| `core` | Logging, assertions, math, file reading, platform detection | — | exists |

## Module layout

```
engine/<module>/
  CMakeLists.txt
  include/mage/<module>/   public headers  -> #include <mage/<module>/File.hpp>
  src/                     sources and private headers
  tests/                   Catch2 unit tests -> mage_<module>_tests
```

## Renderer internals

The public API is small: `Renderer` (create meshes and textures, draw a frame
described by a camera and a list of draw commands), plus the plain data types
`Camera`, `MeshData`, `TextureData` and the procedural `Primitives`.

The Vulkan code lives in `engine/renderer/src/` and is best read in the order
Vulkan is brought up:

| File (`src/vulkan/` unless noted) | Vulkan concept |
|---|---|
| `VulkanApi.hpp` | How MAGE uses Vulkan-Hpp (RAII handles, designated initializers) |
| `VulkanInstance` | Instance, layers, extensions, validation and the debug messenger |
| `VulkanSurface` | The window as a presentation target |
| `VulkanDevice` | Physical device selection, queue families, features, logical device, depth format |
| `VulkanAllocator` | GPU memory allocation with the Vulkan Memory Allocator |
| `VulkanBuffer` | Buffers: GPU-only versus CPU-writable memory |
| `VulkanImage` | Images and image views (textures, depth buffer) |
| `VulkanImageLayout` | Image layouts and pipeline barriers |
| `VulkanImmediateCommands` | One-off command buffers, used for uploads |
| `VulkanSwapchain` | Swapchain images, image views, recreation on resize |
| `VulkanFrame` | Command pool and buffer, semaphore, fence, uniform buffer and descriptor set of one frame in flight |
| `VulkanShaderModule` | Loading SPIR-V |
| `MeshPipeline` | Descriptor set layouts, pipeline layout, vertex input, depth test, graphics pipeline |
| `../RenderResources` | Staging uploads of meshes and textures, descriptor pool, sampler |
| `../Renderer.cpp` | The frame loop: acquire, update uniforms, record, submit, present |

Shader data is organised by how often it changes:

| Frequency | Mechanism | Content |
|---|---|---|
| Per frame | descriptor set 0 (uniform buffer) | view-projection matrix, light direction |
| Per material | descriptor set 1 (combined image sampler) | albedo texture |
| Per object | push constants | model matrix, normal matrix |

Shaders are written in Slang (`shaders/`), compiled to SPIR-V by `slangc` at build
time (`cmake/MageShaders.cmake`) and loaded from the `shaders` folder next to the
executable.

## Conventions

- Right-handed world, +Y up, camera looking along -Z; angles in radians.
- Column-major matrices and column vectors: `clip = projection * view * model * local`.
  The same holds in Slang, where `mul(matrix, vector)` is used.
- Clip space is Vulkan's: Y down (flipped in `Camera::projectionMatrix`), depth 0..1.
- Front faces are counter-clockwise; texture coordinate (0, 0) is the top-left corner.
- Textures are sRGB encoded; shading is done in linear space.

## Third-party dependencies

All dependencies are git submodules in `third_party/`, pinned to release tags and
built from source with the same compiler as the engine.

| Dependency | Version | Used for | In the build |
|---|---|---|---|
| spdlog | v1.17.0 | Logging backend (`core`) | yes |
| Catch2 | v3.16.1 | Unit tests | yes |
| Vulkan-Headers | vulkan-sdk-1.4.363.0 | Vulkan C API and Vulkan-Hpp | yes |
| VulkanMemoryAllocator | v3.5.0 | GPU memory allocation | yes |
| SDL | release-3.4.20 | Window, input, Vulkan surface | yes |
| glm | 1.0.3 | Math | yes |
| imgui | v1.93.0-docking | Editor UI | from editor shell |
| tracy | v0.15.0 | Profiling | when first needed |

The Vulkan SDK is still required on the machine for the validation layers and
the `slangc` shader compiler; the engine itself does not link against it.

## Roadmap

1. **Scaffold** — repository, build system, style tooling, `core`, tests, CI. *(done)*
2. **Vulkan bring-up** — instance, validation, device, swapchain, frame loop, first triangle. *(done)*
3. **GPU resources** — VMA, buffers, textures, descriptors, Slang build step, depth, camera. *(done)*
4. **Editor shell** — ImGui docking, scene rendered offscreen into a viewport panel.
5. **ECS and scene** — custom sparse-set ECS, transforms, serialization, hierarchy and inspector.
6. **Assets** — glTF import, asset handles, content browser.
7. **Rendering features** — PBR, lights, shadows, image-based lighting, HDR, render graph.
8. **Later** — physics, audio, scripting, play mode, shader hot reload.
