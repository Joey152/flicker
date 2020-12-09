#include <GLFW/glfw3.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "graphics/engine.h"
#include "common/linmath.h"

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);

static struct UBO ubo;
static float camera_pos[3] = {0.0f, 0.0f, 0.0f};
static float camera_dir[3] = {0.0f, 0.0f, 1.0f};
static float mouse_yaw = 0.0f;
static float mouse_pitch = 0.0f;
static float xmouse_prev = 0.0f;
static float ymouse_prev = 0.0f;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_W) {
        float forward[3] = {0.0f, 0.0f, 0.1f};
        vec3_add(camera_pos, forward); 
        mat4_view(ubo.view, camera_pos, camera_dir);
    }
    if (key == GLFW_KEY_S) {
        float backward[3] = {0.0f, 0.0f, -0.1f};
        vec3_add(camera_pos, backward); 
        mat4_view(ubo.view, camera_pos, camera_dir);
    }
    if (key == GLFW_KEY_D) {
        float strafe_left[3] = {0.1f, 0.0f, 0.0f};
        vec3_add(camera_pos, strafe_left); 
        mat4_view(ubo.view, camera_pos, camera_dir);
    }
    if (key == GLFW_KEY_A) {
        float strafe_right[3] = {-0.1f, 0.0f, 0.0f};
        vec3_add(camera_pos, strafe_right); 
        mat4_view(ubo.view, camera_pos, camera_dir);
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    printf("%f %f\n", xpos, ypos);
}

int main(void) {
    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(1000, 1000, "Flicker", 0, 0);
    if (!window) {
        // TODO: handle error;
        goto fail_glfw_window;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    gfx_init(window);

    mat4_view(ubo.view, camera_pos, camera_dir);
    mat4_perspective(ubo.proj, 16.0f/9.0f, 90.0f * M_PI / 180.0f, 0.01f, 10.0f);

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

