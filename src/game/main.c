#include <GLFW/glfw3.h>

#include <stdlib.h>

#include "graphics/engine.h"

int main(void) {
    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1000, 1000, "Flicker", 0, 0);
    if (!window) {
        // TODO: handle error;
        goto fail_glfw_window;
    }

    gfx_init(window);

    struct UBO ubo = {
        .view = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        },
        .proj = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        gfx_draw_frame(window, &ubo);
    }

    gfx_deinit();

    glfwDestroyWindow(window);
  fail_glfw_window:

    glfwTerminate();
    return EXIT_SUCCESS;
}

