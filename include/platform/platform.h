#pragma once

#include <volk/volk.h>

#include <stdint.h>

enum ControlState 
{
    CTRL_KEY_STATE_UP,
    CTRL_KEY_STATE_DOWN,
    CTRL_KEY_MAX,
};

enum ControlAction
{
    CTRL_ACTION_PLAYER_FORWARD,
    CTRL_ACTION_PLAYER_BACK,
    CTRL_ACTION_PLAYER_STRAFE_LEFT,
    CTRL_ACTION_PLAYER_STRAFE_RIGHT,
    CTRL_ACTION_MAX,
};

struct ControlEvents
{
    uint64_t player_forward;
    uint64_t player_back;
    uint64_t player_strafe_left;
    uint64_t player_strafe_right;
};

struct Platform 
{
    void (*create_window)(void);
    int (*is_window_terminated)(void);
    void (*poll_events)(void);
    VkResult (*create_surface)(VkInstance instance, VkSurfaceKHR *surface);
    void (*get_window_size)(int *width, int *height);
    void (*get_keyboard_events)(struct ControlEvents *event);
    void (*get_timestamp)(uint64_t *time);
};

extern const struct Platform platform;

