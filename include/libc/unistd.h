#pragma once

#include <stdint.h>
#include <task.h>
#include <irq.h>

enum SEEK_WHENCE {
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

pid_t fork();

void exit(int code);
off_t lseek(int fd, off_t offset, int whence);
ssize_t read(int fd, void *restrict buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);
int close(int fd);
int close_at_task(struct task *t, int fd);
int dup(int fd);

int execve(const char *path, char *const argv[], char *const envp[], struct handler_args *args);