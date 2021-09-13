#pragma once

#include <stdint.h>

#include "graphics/vertex.h"

struct UBO {
    float view[4][4];
    float proj[4][4];
};

struct graphics {
    void (*init)(void);
    void (*deinit)(void);
    void (*draw_frame)(struct UBO *ubo);
    void (*load_map)(uint32_t const size, struct Vertex vertices[static const size]);
};

extern const struct graphics graphics;
