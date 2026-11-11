#pragma once

// The single place where the Vulkan API is included. Every renderer file
// includes this header instead of a Vulkan header directly, so the whole
// module is guaranteed to see the same configuration.
//
// MAGE uses Vulkan-Hpp, the official C++ bindings, in their RAII flavour:
//
//   C API                      Vulkan-Hpp                 Vulkan-Hpp RAII
//   VkDevice                   vk::Device                 vk::raii::Device
//   vkCreateImageView(...)     device.createImageView()   device.createImageView()
//   vkDestroyImageView(...)    device.destroyImageView()  (automatic, in the destructor)
//
//   - `vk::raii::X` OWNS the Vulkan object and destroys it in its destructor.
//     These are what the engine stores as class members.
//   - `vk::X` is a non-owning handle, as cheap to copy as a pointer. Create-info
//     structures and function parameters take these. A `vk::raii::X` converts
//     to its `vk::X` with the dereference operator: `*m_device`.
//   - Names map mechanically, so the Vulkan specification stays usable:
//     drop the `Vk`/`vk` prefix and use the `vk::` namespace; enum values
//     become `vk::Format::eB8G8R8A8Srgb` for `VK_FORMAT_B8G8R8A8_SRGB`.
//   - A failing Vulkan call throws a `vk::SystemError` exception.
//
// Configuration macros (set in the renderer's CMakeLists.txt):
//
//   VK_NO_PROTOTYPES
//     Do not declare the C functions (vkCreateInstance, ...). MAGE never links
//     against the Vulkan library; vk::raii loads each function pointer at
//     runtime, first from the loader library, then per instance and per device.
//
//   VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//     Makes the Vulkan structures plain aggregates, so they can be filled with
//     C++20 designated initializers that name every field being set:
//         vk::ImageViewCreateInfo{ .image = image, .format = format }
//     Fields that are not named are zero-initialised, and `sType` is always
//     filled in automatically.
//
//   VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
//     Acquiring or presenting a swapchain image returns
//     vk::Result::eErrorOutOfDateKHR instead of throwing. An out-of-date
//     swapchain is a normal event (the window was resized), not an error.

#include <vulkan/vulkan_raii.hpp>
