#include <stdint.h>
#include <stdlib.h>

#include "platform/platform.h"
#include "volk/volk.h"
#include "xcb/xcb.h"

xcb_connection_t *connection;
xcb_window_t window;
xcb_screen_t *screen;
xcb_generic_event_t* current_event;

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
    current_event = xcb_wait_for_event(connection);
    return (int)current_event;
}

static void
poll_events(void)
{
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
get_timestamp(uint64_t *time)
{

}

const struct Platform platform = {
    .create_window = create_window,
    .create_surface = create_surface,
    .is_window_terminated = is_window_terminated,
    .poll_events = poll_events,
    .get_window_size = get_window_size,
    .get_keyboard_events = get_keyboard_events,
    .get_timestamp = get_timestamp,
};

