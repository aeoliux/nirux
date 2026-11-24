long syscall(long call, ...);
unsigned int strlen(const char *str);

char *strrev(char *str) {
    unsigned int length = strlen(str);

    for (unsigned int i = 0; i < length / 2; i++) {
        unsigned int j = length - i - 1;

        char temp = str[j];
        str[j] = str[i];
        str[i] = temp;
    }

    return str;
}

char *i32toa(char *dst, unsigned int n, unsigned char base, unsigned char unsig) {
    unsigned int i = 0;
    unsigned char negative = 0;
    
    if (!unsig) {
        negative = !!(n >> 31);
        n = negative ? ~n + 1 : n;
    }

    if (n == 0)
        dst[i++] = '0';
    else {
        while (n != 0) {
            unsigned int rem = n % base;
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

int puts(const char *str) {
    return (int) syscall(4, 1, str, strlen(str));
}