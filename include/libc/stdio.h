#pragma once

#include <stdint.h>

void putchar(uint8_t ch);
void puts(const char *str);
void printf(const char *fmt, ...);
void vprintf(const char *fmt, uint8_t *arg);
char *i32toa(char *dst, uint32_t n, uint8_t base, uint8_t unsig);