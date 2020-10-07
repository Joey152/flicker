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

    struct gfx_engine* graphics_engine = 0;
    gfx_init(graphics_engine);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
  fail_glfw_window:

    glfwTerminate();
    return EXIT_SUCCESS;
}

