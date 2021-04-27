#pragma once

struct UBO {
    float view[4][4];
    float proj[4][4];
};

struct graphics {
    void (*init)(void);
    void (*deinit)(void);
    void (*draw_frame)(struct UBO *ubo);
};

extern const struct graphics graphics;

