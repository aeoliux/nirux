#pragma once

#include <stdint.h>

enum SIGNAL {
    SIGHUP = 1,
    SIGINT,
    SIGQUIT,
    SIGILL,
    SIGTRAP,
    SIGABRT,
    SIGBUS,
    SIGFPE,
    SIGKILL,
    SIGUSR1,
    SIGSEGV,
    SIGUSR2,
    SIGPIPE,
    SIGALRM,
    SIGTERM,
    SIGSTKFLT,
    SIGCHLD,
    SIGCONT,
    SIGSTOP,
    SIGTSTP,
    SIGTTIN,
    SIGTTOU,
    SIGURG,
    SIGXCPU,
    SIGXFSZ,
    SIGVTALRM,
    SIGPROF,
    SIGWINCH,
    SIGPOLL,
    SIGPWR,
    SIGSYS,

    __SIGCOUNT // NOT A SIGNAL
};

#define SIGCOUNT (__SIGCOUNT - 1)
#define SIGIOT SIGABRT
#define SIGCLD SIGCHLD
#define SIGIO SIGPOLL

#define SIG_DFL ((sighandler_t) 0)
#define SIG_IGN ((sighandler_t) 1)

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
