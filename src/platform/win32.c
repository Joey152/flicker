#include <windows.h>
#include <volk/volk.h>

#include <stdio.h>

#include "platform/platform.h"

HINSTANCE window_instance;
HWND window_handle;
MSG msg;
struct ControlEvents start_events;
struct ControlEvents end_events;

WPARAM control_action_mapping[CTRL_ACTION_MAX] = {
    0x57,
    0x53,
    0x41,
    0x44,
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void
create_window(void)
{
    const char CLASS_NAME[] = "Flicker";

    window_instance = GetModuleHandle(0);

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = 0,
        .lpfnWndProc = WindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = window_instance,
        .hIcon = 0,
        .hCursor = 0,
        .hbrBackground = 0,
        .lpszMenuName = 0,
        .lpszClassName = CLASS_NAME,
        .hIconSm = 0,
    };

    RegisterClassEx(&wc);

    window_handle = CreateWindowEx(
        0,
        CLASS_NAME,
        0,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        0,
        0,
        window_instance,
        0
    );
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN:
        {
            int is_initial_key_press = lParam & (1 << 30);
            if (!is_initial_key_press) {
                printf("WM_KEYDOWN: %llx\n", wParam);
                if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_FORWARD])
                {
                    printf("go forward\n");
                } 
                else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_BACK])
                {
                    printf("go back\n");
                }
                else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_STRAFE_LEFT])
                {
                    printf("go left\n");
                }
                else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_STRAFE_RIGHT])
                {
                    printf("go right\n");
                }
            }

            return 0;
        }
        case WM_KEYUP:
        {
            printf("WM_KEYUP: %llx\n", wParam);
            if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_FORWARD])
            {
                printf("go forward\n");
                //get_timestamp(&end_events.player_forward);
            } 
            else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_BACK])
            {
                printf("go back\n");
                //get_timestamp(&end_events.player_back);
            }
            else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_STRAFE_LEFT])
            {
                printf("go left\n");
                //get_timestamp(&end_events.player_strafe_left);
            }
            else if (wParam == control_action_mapping[CTRL_ACTION_PLAYER_STRAFE_RIGHT])
            {
                printf("go right\n");
                //get_timestamp(&end_events.player_strafe_right);
            }

            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static VkResult
create_surface(VkInstance instance, VkSurfaceKHR *surface)
{
    VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = window_instance,
        .hwnd = window_handle,
    };

    return vkCreateWin32SurfaceKHR(instance, &create_info, 0, surface);
}

static int
is_window_terminated(void)
{
    while (PeekMessage(&msg, window_handle, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return 0;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 1;
}

static void
poll_events(void)
{
    //printf("%u\n", msg.message);
}

static void
get_window_size(int *width, int *height)
{
    RECT rect;
    GetClientRect(window_handle, &rect);

    *width = rect.right;
    *height = rect.bottom;
}

static void
get_keyboard_events(struct ControlEvents *events) 
{
    events->player_forward = start_events.player_forward;
    events->player_back = start_events.player_back;
    events->player_strafe_left = start_events.player_strafe_left;
    events->player_strafe_right = start_events.player_strafe_right;

    begin
}

static void
get_timestamp(uint64_t *time)
{
    LARGE_INTEGER l;
    QueryPerformanceCounter(&l);

    *time = l.QuadPart;
}

/* Export Window Library */
const struct Platform window = {
    .create_window = create_window,
    .create_surface = create_surface,
    .is_window_terminated = is_window_terminated,
    .poll_events = poll_events,
    .get_window_size = get_window_size,
    .get_keyboard_events = get_keyboard_events,
    .get_timestamp = get_timestamp,
};

