#include "graphics/resource.h"

#include <vulkan/vulkan.h>

#include <assert.h>

uint32_t gfx_get_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties); 

    for (size_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    assert(0);
}

void gfx_destroy_resource(VkDevice device, struct GfxResource *resource, VkAllocationCallbacks const *allocator) {
    vkFreeMemory(device, resource->memory, allocator);
    vkDestroyBuffer(device, resource->buffer, allocator);
}
