#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct gfx_engine {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDevice device;
};

int gfx_init(struct gfx_engine engine[static 1], GLFWwindow *window);

