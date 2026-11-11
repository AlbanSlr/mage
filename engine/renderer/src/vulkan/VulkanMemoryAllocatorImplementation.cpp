// The Vulkan Memory Allocator is a "single header" library: vk_mem_alloc.h
// contains both its declarations and its implementation. The implementation is
// only compiled in the one source file that defines VMA_IMPLEMENTATION before
// including the header. This is that file, and it contains nothing else.

#include "vulkan/VulkanApi.hpp" // IWYU pragma: keep

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
