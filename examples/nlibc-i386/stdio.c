long syscall(long call, ...);
unsigned int strlen(const char *str);

int puts(const char *str) {
    return (int) syscall(4, 1, str, strlen(str));
}