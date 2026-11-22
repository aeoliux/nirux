#pragma once

#include <stdint.h>

enum WAITPID_OPTS {
    WNOHANG
};

pid_t waitpid(pid_t pid, int *wstatus, int options);