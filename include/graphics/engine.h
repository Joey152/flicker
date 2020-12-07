#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct UBO {
    float view[4][4];
    float proj[4][4];
};

int gfx_init(GLFWwindow *window);

void gfx_deinit();

void gfx_draw_frame(GLFWwindow *window, struct UBO *ubo);

