#include <irq.h>
#include <fcntl.h>
#include <task.h>
#include <ssb.h>

int execve(const char *path, char *const argv[], char *const envp[], struct handler_args *args) {
    int fd = open(path, O_EXEC);
    if (fd < 0) return fd;

    int result = exec_ssb(SSB_SRC_FILE, (uword_t) fd, (const char **) argv, (const char **) envp, args);
    return result;
}