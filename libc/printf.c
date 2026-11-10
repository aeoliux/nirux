#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <termdrv.h>

void vprintf(const char *fmt, uint8_t *arg) {
    uint8_t operand = 0;
    char convbuf[64] = {0};

    for (; *fmt; fmt++) {
        if (!operand) {
            if (*fmt == '%')
                operand = 1;
            else
                putchar(*fmt);
        } else {
            switch (*fmt) {
                case 'u':
                    i32toa(convbuf, *(uint32_t *) arg, 10, 1);
                    arg += (uword_t) sizeof(uint32_t);
                    puts(convbuf);
                    break;
                case 'd':
                    i32toa(convbuf, *(uint32_t *) arg, 10, 0);
                    arg += (uword_t) sizeof(uint32_t);
                    puts(convbuf);
                    break;
                case 'c':
                    putchar(*(char *) arg);
                    arg += (uword_t) sizeof(char);
                    break;
                case 'x':
                    i32toa(convbuf, *(uint32_t *) arg, 16, 1);
                    arg += (uword_t) sizeof(uint32_t);
                    puts(convbuf);
                    break;
                case 'b':
                    i32toa(convbuf, (uint32_t) *((uint8_t *) arg), 16, 1);
                    arg += (uword_t) sizeof(uint8_t);
                    puts(convbuf);
                    break;
                case 's':
                    puts(*((char **) arg));
                    arg += (uword_t) sizeof(char *);
                    break;
                case '%':
                    putchar('%');
                    break;
            }

            memset16(convbuf, 0, sizeof(convbuf) / 2);
            operand = 0;
        }
    }
}

void printf(const char *fmt, ...) {
    uint8_t *arg = (uint8_t *)&fmt + (uword_t) sizeof(fmt);

    vprintf(fmt, arg);
}

char *i32toa(char *dst, uint32_t n, uint8_t base, uint8_t unsig) {
    uint32_t i = 0;
    uint8_t negative = 0;
    
    if (!unsig) {
        negative = !!(n >> 31);
        n = negative ? ~n + 1 : n;
    }

    if (n == 0)
        dst[i++] = '0';
    else {
        while (n != 0) {
            uint32_t rem = n % base;
            dst[i++] = (rem > 9) ? rem - 10 + 'a' : rem + '0';
            n /= base;
        }
    }

    if (base == 16) {
        dst[i++] = 'x';
        dst[i++] = '0';
    }

    if (negative)
        dst[i++] = '-';

    dst[i] = '\0';

    strrev(dst);

    return dst;
}

void putchar(uint8_t ch) {
    terminal_interface.write(4, 0, 0, &ch, 1);
}

void puts(const char *str) {
    terminal_interface.write(4, 0, 0, str, strlen(str));
}