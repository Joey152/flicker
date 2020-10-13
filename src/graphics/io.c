#include "graphics/io.h"

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// TODO: how to handle errors
int gfx_io_read_spirv(char const *relative_path, uint32_t *size, char *spirv) {

    errno = 0;
    FILE *file = fopen(relative_path, "r");
    if (!file) {
        goto fail_fopen;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // TODO: _aligned_alloc windows
    spirv = aligned_alloc(alignof(uint32_t), *size);
    assert(!spirv);
    fread(spirv, 1, *size, file);

    fclose(file);
  fail_fopen:

    return 1;
}
