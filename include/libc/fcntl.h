#pragma once

#include <task.h>

enum file_access_modes {
    O_RDONLY        = 0x1,
    O_WRONLY        = 0x2,
    O_RDWR          = O_RDONLY | O_WRONLY,
    O_APPEND        = O_WRONLY | 0x4,
    O_CREAT         = O_RDWR | 0x8,
    O_DIRECTORY     = 0x10,

    O_EXEC          = O_RDONLY | 0x100,
};

int open(const char *path, int flags, ...);
int open_at_task(struct task *t, const char *path, int flags, uint32_t mode);