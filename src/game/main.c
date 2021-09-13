#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "game/io.h"
#include "graphics/graphics.h"
#include "graphics/vertex.h"
#include "common/linmath.h"
#include "platform/platform.h"

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static struct UBO ubo;
static float camera_pos[3] = {0.0f, 9.5f, 0.0f};
static float camera_dir[3] = {0.0f, 0.0f, 1.0f};
static float mouse_yaw = 0.0f;
static float mouse_pitch = 0.0f;
static double xmouse_prev = 0.0f;
static double ymouse_prev = 0.0f;
static struct PlayerControlEvent control_event;

int
main(void)
{
    platform.create_window();

    graphics.init();

    char const *map1 = "asset/mesh/map1.vertex";
    FILE *file = fopen(map1, "rb");
    uint32_t vertex_count;
    io_load_mesh(file, &vertex_count, 0);
    struct Vertex *vertices = malloc(vertex_count * sizeof *vertices);
    io_load_mesh(file, &vertex_count, vertices);

    graphics.load_map(vertex_count, vertices);

    mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    mat4_perspective(ubo.proj, 16.0f/9.0f, 90.0f * M_PI / 180.0f, 0.01f, 1000.0f);

    platform.init_timestamp();
    while (platform.is_application_running())
    {
        platform.poll_events();
        platform.get_keyboard_events(&control_event);
        vec3_add(camera_pos, control_event.forward_time * 0.0000001f, control_event.strafe_time * 0.0000001f, 0.0f);
        mouse_pitch = control_event.mouse_y / 100.0;
        mouse_yaw = control_event.mouse_x / 100.0;
        mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
        graphics.draw_frame(&ubo);
    }

    free(vertices);
    fclose(file);

    graphics.deinit();

    return EXIT_SUCCESS;
}
