#pragma once

#include <vulkan/vulkan.h>

struct gfx_engine {
    VkInstance instance;
    VkDevice device;
};

int gfx_init(struct gfx_engine engine[static 1]);

