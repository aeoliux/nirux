#pragma once

enum MOUNT_FLAGS {
    MS_RDONLY       = 0x1,
};

int fmount(int fd, const char *target, const char *fstype, unsigned long mountflags, const void *fsflags);
int mount(const char *dev, const char *target, const char *fstype, unsigned long mountflags, const void *fsflags);