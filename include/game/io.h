#pragma once

#include <stdio.h>
#include <stdint.h>

#include <graphics/vertex.h>

void
io_load_mesh(FILE *file, uint32_t *count, struct Vertex *vertices);
