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