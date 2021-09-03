#pragma once

#include <stddef.h>
#include "vertex.h"

struct Triangles {
    size_t length;
    struct Vertex *vertices;
    float *centroids;
};
