#pragma once

#include <stdint.h>

void *memset(void *restrict buf, int c, size_t n);
void *memset16(void *restrict buf, uint16_t word, size_t n);
void *memcpy(void *restrict dst, const void *src, size_t n);