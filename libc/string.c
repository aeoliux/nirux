#include <stdint.h>

void *memset(void *restrict buf, int c, size_t n) {
    for (uword_t i = 0; i < n; i++) ((uint8_t *) buf)[i] = c;

    return buf;
}

void *memset16(void *restrict buf, uint16_t word, size_t n) {
    for (uword_t i = 0; i < n; i++) ((uint16_t *)buf)[i] = word;

    return buf;
}

void *memcpy(void *restrict dst, const void *src, size_t n) {
    for (uword_t i = 0; i < n; i++) ((uint8_t *) dst)[i] = ((uint8_t *) src)[i];

    return dst;
}