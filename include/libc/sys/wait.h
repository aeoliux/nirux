#pragma once

#include <stdint.h>

#define WIFEXITED(status)       ((status >> 8) & 0x1)
#define WEXITSTATUS(status)     (status & 0xff)

enum WAITPID_OPTS {
    WNOHANG         = 0x1
};

pid_t waitpid(pid_t pid, int *wstatus, int options);
