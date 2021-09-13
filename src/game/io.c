#include "game/io.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "graphics/vertex.h"

void
io_load_mesh(FILE *file, uint32_t *count, struct Vertex *vertices)
{
    assert(file);
    assert(count);

    if (vertices)
    {
        fread(vertices, sizeof *vertices, *count, file);
    }
    else
    {
        fread(count, sizeof *count, 1, file);
    }
}
