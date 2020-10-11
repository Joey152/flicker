#pragma once

#include <vulkan/vulkan.h>

struct GfxResource {
    VkBuffer buffer;
    VkDeviceMemory memory;
};

uint32_t gfx_get_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags flags);

void gfx_destroy_resource(VkDevice device, struct GfxResource *resource, VkAllocationCallbacks const *allocator);

