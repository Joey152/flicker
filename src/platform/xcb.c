#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "platform/platform.h"
#include "volk/volk.h"
#include "xcb/xcb.h"

#define WM_PROTOCOLS "WM_PROTOCOLS"
#define WM_DELETE_WINDOW "WM_DELETE_WINDOW"

enum
PLAYER_ACTIONS_MOVEMENT
{
    PLAYER_ACTIONS_MOVEMENT_FORWARD,
    PLAYER_ACTIONS_MOVEMENT_BACKWARD,
    PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT,
    PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT,
    PLAYER_ACTIONS_MOVEMENT_MAX,
};

int is_app_running = 1;
xcb_connection_t *connection;
xcb_window_t window;
xcb_screen_t *screen;
xcb_generic_event_t *current_event;
xcb_intern_atom_cookie_t wm_protocols_cookie;
xcb_intern_atom_reply_t *wm_protocols;
xcb_intern_atom_cookie_t wm_delete_window_cookie;
xcb_intern_atom_reply_t *wm_delete_window;
struct timespec current_time;
struct PlayerControlEvent player_control_event;
int player_control_end_state[PLAYER_ACTIONS_MOVEMENT_MAX];
long player_control_forward_begin;
long player_control_forward_end;
long player_control_backward_begin;
long player_control_backward_end;
long player_control_strafe_right_begin;
long player_control_strafe_right_end;
long player_control_strafe_left_begin;
long player_control_strafe_left_end;
long prev_time;

uint8_t player_actions_movement_mapping[] = {
    0x19,
    0x27,
    0x28,
    0x26,
};

static void
get_timestamp(long *time);

static long
time_diff_in_ns(struct timespec t1, struct timespec t2)
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

    wm_protocols_cookie = xcb_intern_atom(connection, 1, sizeof WM_PROTOCOLS - 1, WM_PROTOCOLS);
    wm_protocols = xcb_intern_atom_reply(connection, wm_protocols_cookie, 0);
    wm_delete_window_cookie = xcb_intern_atom(connection, 0, sizeof WM_DELETE_WINDOW - 1, WM_DELETE_WINDOW);
    wm_delete_window = xcb_intern_atom_reply(connection, wm_delete_window_cookie, 0);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, wm_protocols->atom, 4, 32, 1, &wm_delete_window->atom);

    xcb_map_window(connection, window);
    xcb_flush(connection);
}

static int
is_application_running(void)
{
    return is_app_running;
}

static void
poll_events(void)
{
    // TODO: how to choose a proper array length
    #define MAX_POLL_EVENTS 100
    static xcb_generic_event_t *events[MAX_POLL_EVENTS];

    for (size_t i = 0; i < MAX_POLL_EVENTS; i++)
    {
        events[i] = xcb_poll_for_event(connection);
        if (!events[i]) break;
    }

    for (size_t i = 0; i < MAX_POLL_EVENTS; i++)
    {
        if (!events[i]) break;

        switch (events[i]->response_type & ~0x80)
        {
            case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t *event = (xcb_client_message_event_t *)events[i];
                if (event->data.data32[0] == wm_delete_window->atom)
                {
                    is_app_running = 0;
                }
                break;
            }
            case XCB_KEY_PRESS:
            {
                xcb_key_press_event_t *event = (xcb_key_press_event_t *)events[i];
                if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_FORWARD])
                {
                    get_timestamp(&player_control_forward_begin);
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_FORWARD] += 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_BACKWARD])
                {
                    get_timestamp(&player_control_backward_begin);
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_BACKWARD] += 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT])
                {
                    get_timestamp(&player_control_strafe_right_begin);
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT] += 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT])
                {
                    get_timestamp(&player_control_strafe_left_begin);
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT] += 1;
                }

                break;
            }
            case XCB_KEY_RELEASE:
            {
                xcb_key_release_event_t *event = (xcb_key_release_event_t *)events[i];

                // if next event is key press this indicates that the user has held down the key
                if (i+1 < MAX_POLL_EVENTS && events[i+1])
                {
                    if ((events[i+1]->response_type & ~0x80) == XCB_KEY_PRESS)
                    {
                        xcb_key_press_event_t *next_event = (xcb_key_press_event_t *)events[i+1];
                        if (event->detail == next_event->detail)
                        {
                            // skip to event after press event
                            i += 1;
                            events[i+1] = 0;
                            continue;
                        }
                    }
                }

                if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_FORWARD])
                {
                    get_timestamp(&player_control_forward_end);
                    player_control_event.forward_time += player_control_forward_end - player_control_forward_begin;
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_FORWARD] -= 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_BACKWARD])
                {
                    get_timestamp(&player_control_backward_end);
                    player_control_event.forward_time -= player_control_backward_end - player_control_backward_begin;
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_BACKWARD] -= 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT])
                {
                    get_timestamp(&player_control_strafe_right_end);
                    player_control_event.strafe_time += player_control_strafe_right_end - player_control_strafe_right_begin;
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT] -= 1;
                }
                else if (event->detail == player_actions_movement_mapping[PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT])
                {
                    get_timestamp(&player_control_strafe_left_end);
                    player_control_event.strafe_time -= player_control_strafe_left_end - player_control_strafe_left_begin;
                    player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT] -= 1;
                }
            }
            case XCB_BUTTON_RELEASE:
            {
                xcb_key_release_event_t *event = (xcb_key_release_event_t *)events[i];
                break;
            }
            case XCB_BUTTON_PRESS:
            {
                xcb_button_press_event_t *event = (xcb_button_press_event_t *)events[i];
                break;
            }
            default:
            {
                break;
            }
        }

        events[i] = 0;
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
get_keyboard_events(struct PlayerControlEvent *event)
{
    long cutoff;
    get_timestamp(&cutoff);
    if (player_control_end_state[PLAYER_ACTIONS_MOVEMENT_FORWARD]) {
        player_control_event.forward_time += cutoff - player_control_forward_begin;
        if (player_control_event.forward_time < 0) {
            printf("cut:%ld begin:%ld \n", cutoff, player_control_forward_begin);
        }
    }
    if (player_control_end_state[PLAYER_ACTIONS_MOVEMENT_BACKWARD]) {
        player_control_event.forward_time -= cutoff - player_control_backward_begin;
    }
    if (player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_RIGHT]) {
        player_control_event.strafe_time += cutoff - player_control_strafe_right_begin;
    }
    if (player_control_end_state[PLAYER_ACTIONS_MOVEMENT_STRAFE_LEFT]) {
        player_control_event.strafe_time -= cutoff - player_control_strafe_left_begin;
    }

    event->forward_time = player_control_event.forward_time;
    event->strafe_time = player_control_event.strafe_time;

    player_control_forward_begin = cutoff;
    player_control_backward_begin = cutoff;
    player_control_strafe_right_begin = cutoff;
    player_control_strafe_left_begin = cutoff;
    player_control_forward_end = 0;
    player_control_backward_end = 0;
    player_control_strafe_right_end = 0;
    player_control_strafe_left_end = 0;
    player_control_event.forward_time = 0;
    player_control_event.strafe_time = 0;
}

static void
get_timestamp(long *time)
{
    struct timespec temp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &temp);

    *time = temp.tv_sec * 1000000 + temp.tv_nsec;
}

static void
init_timestamp(void)
{
    long time;
    get_timestamp(&time);

    player_control_forward_begin = time;
    player_control_backward_begin = time;
    player_control_strafe_right_begin = time;
    player_control_strafe_left_begin = time;
}

const struct Platform platform = {
    .create_window = create_window,
    .create_surface = create_surface,
    .is_application_running = is_application_running,
    .poll_events = poll_events,
    .get_window_size = get_window_size,
    .get_keyboard_events = get_keyboard_events,
    .get_timestamp = get_timestamp,
    .init_timestamp = init_timestamp,
};
