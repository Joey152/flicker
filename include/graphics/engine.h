#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <graphics/vertex.h>

struct UBO {
    float view[4][4];
    float proj[4][4];
};

int gfx_init(GLFWwindow *window, size_t vertices_size, uint32_t buffer_sizes[vertices_size], char const *const vertices[vertices_size]);

void gfx_deinit();

void gfx_draw_frame(GLFWwindow *window, struct UBO *ubo);

