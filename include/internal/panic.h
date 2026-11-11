#pragma once

#include <stdint.h>

void panic(const char *fmt, uint32_t *eip, ...);