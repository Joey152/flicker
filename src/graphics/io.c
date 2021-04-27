#include "graphics/io.h"

#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "graphics/resource.h"

// TODO: how to handle errors
int
io_read_spirv(char const *relative_path, uint32_t *size, uint32_t **spirv)
{
    FILE *file;
    errno_t err = fopen_s(&file, relative_path, "rb");
    if (!file) {
        goto fail_fopen;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(*size);

#ifdef _WIN32
    *spirv = _aligned_malloc(*size, alignof(uint32_t));
#else
#error Unsupported OS
#endif

    assert(spirv != 0);
    size_t s = fread(*spirv, 1, *size, file);
    assert(s != 0);

    fclose(file);
  fail_fopen:

    return 1;
}

int
io_read_static_vertices(char const *relative_path, struct GfxResource *vertex) {
    FILE *file;
    fopen_s(&file, relative_path, "r");
    if (!file) {
        goto fail_open;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    assert(size != -1L);

    char *buffer = malloc(size);
    assert(buffer != 0);
    size_t read_size = fread(buffer, 1, size, file);
    assert(read_size == size);

    fclose(file);
  fail_open:

    return 1;
}
