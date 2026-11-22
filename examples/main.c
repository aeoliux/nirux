extern char **environ;

long syscall(long call, ...);

unsigned int strlen(const char *str) {
    unsigned int i = 0;
    for (; str[i]; i++);
    return i;
}

int strncmp(const char *s1, const char *s2, unsigned int n) {
    unsigned int i = 0;

    for (; i < n && s1[i] && s2[i]; i++)
        if (s1[i] - s2[i])
            return s1[i] - s2[i];

    return i >= n ? 0 : s1[i] - s2[i];
}

int strcmp(const char *s1, const char *s2) {
    return strncmp(s1, s2, strlen(s1) + 1);
}

int main(int argc, const char **argv) {
    syscall(4, 1, "Printing args...\n", 17);

    char nl = '\n';
    for (unsigned int i = 0; i < argc; i++) {
        syscall(4, 1, argv[i], strlen(argv[i]));

        syscall(4, 1, &nl, 1);
    }

    if (argc > 0 && strcmp(argv[0], "stop")) {
        const char *argv[] = {"stop", 0};

        syscall(11, "/bin/a.ssb", argv, environ);
    }

    return argc;
}