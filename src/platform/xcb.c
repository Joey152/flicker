#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "platform/platform.h"
#include "volk/volk.h"
#include "xcb/xcb.h"

int is_app_running = 1;
xcb_connection_t *connection;
xcb_window_t window;
xcb_screen_t *screen;
xcb_generic_event_t* current_event;
struct timespec prev_time;
struct timespec current_time;

enum
PLAYER_ACTIONS_MOVEMENT 
{
    PLAYER_ACTIONS_MOVEMENT_FORWARD,
    PLAYER_ACTIONS_MOVEMENT_BACKWARD,
    PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT,
    PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT,
};

uint8_t player_actions_movement_mapping[] = {

};

static long time_diff_in_ns(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    long ns = t2.tv_nsec - t1.tv_nsec;
    if (ns < 0)
    {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = ns + 1000000000;
    }
    else
    {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = ns;
    }

    return (diff.tv_sec * 1000000000.0 + diff.tv_nsec);
}

static void
create_window(void)
{
    connection = xcb_connect(0, 0);
    // if (xcb_connection_has_error(connection))
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    window = xcb_generate_id(connection);

    int windowHeight = 1000;
    int windowWidth = 1000;
    uint32_t value_mask = XCB_CW_EVENT_MASK;
    uint32_t value_list[] = {
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    };

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        window,
        screen->root,
        0,
        0,
        windowWidth,
        windowHeight,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask,
        value_list
    );

    xcb_map_window(connection, window);
    xcb_flush(connection);
}


static int 
is_window_terminated(void)
{
    current_event = xcb_poll_for_event(connection);
    return (int)current_event;
}

static int
is_application_running(void)
{
    return is_app_running;
}

static void
poll_events(void)
{
    current_event = xcb_poll_for_event(connection);
    if (current_event) {
        switch (current_event->response_type)
        {
            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t *event = (xcb_key_press_event_t *)current_event;
                printf("%x\n", event->detail);
            }
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *event = (xcb_button_press_event_t *)current_event;
                //printf("%x\n", event->detail);
            }
            default:
            {
                //printf("%u\n", current_event->response_type);
                break;
            }
        }
    }
}

static VkResult
create_surface(VkInstance instance, VkSurfaceKHR *surface)
{
    VkXcbSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = 0,
        .flags = 0,
        .connection = connection,
        .window = window,
    };

    return vkCreateXcbSurfaceKHR(instance, &create_info, 0, surface);
}

static void get_window_size(int *width, int *height)
{
    xcb_get_geometry_reply_t *g = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, window), 0);

    *width = g->width;
    *height = g->height;

    free(g);
}

static void
get_keyboard_events(struct ControlEvents *event)
{
    
}

static void
get_timestamp(struct timespec *time)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, time);

    struct timespec temp = {
        .tv_sec = time->tv_sec,
        .tv_nsec = time->tv_nsec,
    };

    time->tv_sec = time->tv_sec - prev_time.tv_sec;
    time->tv_nsec = time->tv_nsec - prev_time.tv_nsec;

    prev_time.tv_sec = temp.tv_sec;
    prev_time.tv_nsec = temp.tv_nsec;
}

static long
get_delta_time(void)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
    long diff = time_diff_in_ns(prev_time, current_time);
    prev_time = current_time;

    return diff;
}

static void
init_timestamp(void)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &prev_time);
}

const struct Platform platform = {
    .create_window = create_window,
    .create_surface = create_surface,
    .is_window_terminated = is_window_terminated,
    .is_application_running = is_application_running,
    .poll_events = poll_events,
    .get_window_size = get_window_size,
    .get_keyboard_events = get_keyboard_events,
    .get_timestamp = get_timestamp,
    .get_delta_time = get_delta_time,
    .init_timestamp = init_timestamp,
};

