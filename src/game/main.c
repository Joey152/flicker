#include <inttypes.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "graphics/graphics.h"
#include "common/linmath.h"
#include "window/window.h"

static struct UBO ubo;
static float camera_pos[3] = {0.0f, 0.0f, 0.0f};
static float camera_dir[3] = {0.0f, 0.0f, 1.0f};
static float mouse_yaw = 0.0f;
static float mouse_pitch = 0.0f;
static double xmouse_prev = 0.0f;
static double ymouse_prev = 0.0f;
static struct ControlEvents control_events;

int
main(void)
{
    window.create_window();

    graphics.init();

    mat4_view(ubo.view, camera_pos, mouse_pitch, mouse_yaw);
    mat4_perspective(ubo.proj, 16.0f/9.0f, 90.0f * M_PI / 180.0f, 0.01f, 1000.0f);

    uint64_t time;
    while (window.is_window_terminated())
    {
        graphics.draw_frame(&ubo);
        window.poll_events();
        window.get_keyboard_events(&control_events);
        //printf("W:%u A:%u S:%u D:%u\n", control_events.player_forward, control_events.player_back, control_events.player_strafe_left, control_events.player_strafe_right);
    }

    graphics.deinit();

    return EXIT_SUCCESS;
}

