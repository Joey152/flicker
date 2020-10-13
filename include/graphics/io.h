#pragma once

#include <errno.h>
#include <stdint.h>

int gfx_io_read_spirv(char const *relative_path, uint32_t *size, char *spirv);

