#include <GLFW/glfw3.h>

#include <assert.h>
#include <inttypes.h>
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
static double xmouse_prev = 0.0f;
static double ymouse_prev = 0.0f;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    float cos_yaw = cosf(mouse_yaw);
    float sin_yaw = sinf(mouse_yaw);
    float cos_pitch = cosf(mouse_pitch);
    float forward[3] = {sin_yaw * cos_pitch, 0.0f, cos_yaw * cos_pitch};
    if (key == GLFW_KEY_W) {
        vec3_add(camera_pos, forward); 
        mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    }
    if (key == GLFW_KEY_S) {
        float backward[3] = {-forward[0],0.0f,-forward[2]};
        vec3_add(camera_pos, backward); 
        mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    }
    float strafe_right[3]; 
    float up[3] = {0.0f, -1.0f, 0.0f};
    vec3_cross(strafe_right, forward, up);
    if (key == GLFW_KEY_D) {
        vec3_add(camera_pos, strafe_right); 
        mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    }
    if (key == GLFW_KEY_A) {
        float strafe_left[3] = {-strafe_right[0], 0.0f, -strafe_right[2]};
        vec3_add(camera_pos, strafe_left); 
        mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {

    float inc_yaw = mouse_yaw - (0.01f * (xmouse_prev - xpos));
    float inc_pitch = mouse_pitch - (0.01f * (ymouse_prev - ypos));

    if (inc_pitch >= -M_PI/2.0f && inc_pitch <= M_PI/2.0f) {
        mouse_pitch = inc_pitch;
    }

    mouse_yaw = inc_yaw;
    mouse_yaw = fmod(mouse_yaw, 2 * M_PI);

    printf("angles: %f %f\n", mouse_yaw, mouse_pitch);

    mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);

    xmouse_prev = xpos;
    ymouse_prev = ypos;
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported() != 0) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwGetCursorPos(window, &xmouse_prev, &ymouse_prev);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    char const *const persistent_mesh_vertices[2] = {
        "asset/mesh/map1.vertex",
        "asset/mesh/monkey.vertex",
    };

    uint32_t persistent_mesh_sizes[2];
    uint32_t num_of_vertices[2];
    for (size_t i = 0; i < 2; i++) {
        FILE *file = fopen(persistent_mesh_vertices[i], "rb");
        fread(&num_of_vertices[i], sizeof *num_of_vertices, 1, file);
        persistent_mesh_sizes[i] = num_of_vertices[i] * sizeof(float) * 3 * 6;
        fclose(file);
    }

    gfx_init(window, 2, persistent_mesh_sizes, persistent_mesh_vertices);

    mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    mat4_perspective(ubo.proj, 16.0f/9.0f, 90.0f * M_PI / 180.0f, 0.01f, 1000.0f);

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

