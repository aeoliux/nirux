#pragma once

#include <stdint.h>

void *memset(void *restrict buf, int c, size_t n);
void *memset16(void *restrict buf, uint16_t word, size_t n);
void *memcpy(void *restrict dst, const void *src, size_t n);

uint32_t strlen(const char * data);

char *strrev(char *str);
char *strcpy(char *restrict dst, const char *src);
char *strncat(char *restrict dst, const char *src, size_t n);

int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);