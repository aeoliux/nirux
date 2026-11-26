#include "../../include/internal/fs/uxfat.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2)
        return 1;

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open()");
        return 1;
    }

    struct fs_uxfat_superblock sblk;
    if (read(fd, &sblk, 62) < 62) {
        perror("read(superblock)");
        close(fd);
        return 1;
    }

    if (
        sblk.legacy_spf || sblk.signature != 0x2a || !(sblk.fat_addressing & 0xf0) || strncmp((char *) sblk.system_id, "UXFAT   ", 8)
    ) {
        printf("invalid UXFAT filesystem\n");
        close(fd);
        return 1;
    }

    char cwd[PATH_MAX + 1];
    if (argv[1][0] != '/' && !getcwd(cwd, PATH_MAX - strlen(argv[1]))) {
        perror("getcwd()");
        close(fd);
        return 1;
    }

    strcat(cwd, argv[1]);
    cwd[strlen(cwd)] = 0;

    int out = open("/tmp/uxmount", O_WRONLY | O_CREAT, 0644);
    if (out < 0) {
        perror("open(out)");
        close(fd);
        return 1;
    }

    write(out, &sblk, 62);
    write(out, cwd, strlen(cwd) + 1);

    close(out);
    close(fd);
}