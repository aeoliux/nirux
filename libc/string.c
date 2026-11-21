#include <stdint.h>
#include <string.h>

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

char *strrev(char *str) {
    size_t length = strlen(str);

    for (uint32_t i = 0; i < length / 2; i++) {
        size_t j = length - i - 1;

        char temp = str[j];
        str[j] = str[i];
        str[i] = temp;
    }

    return str;
}

uint32_t strlen(const char * data) {
    uint32_t len = 0;
    for (; data[len]; len++);
    return len;
}

char *strcpy(char *restrict dst, const char *src) {
    return memcpy(dst, src, strlen(src));
}

char *strncat(char *restrict dst, const char *src, size_t n) {
    memcpy(dst + strlen(dst), src, n);

    dst[strlen(dst)] = '\0';
    return dst;
}

char *strcat(char *restrict dst, const char *src) {
    return strncat(dst, src, strlen(src));
}

int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i = 0;

    for (; i < n && s1[i] && s2[i]; i++)
        if (s1[i] - s2[i])
            return s1[i] - s2[i];

    return i >= n ? 0 : s1[i] - s2[i];
}

int strnmis(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++)
        if (s1[i] - s2[i])
            return (int) i;
    
    return -1;
}

int strcmp(const char *s1, const char *s2) {
    return strncmp(s1, s2, strlen(s1) + 1);
}