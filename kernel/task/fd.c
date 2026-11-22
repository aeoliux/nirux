#include <kmem.h>
#include <fs.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <errno.h>
#include <unistd.h>

static inline int am_i_allowed(int type, struct task *t, struct fs_direntry *f) {
    if (t->is_kthread)
        return 1;

    uint16_t mode = f->mode;

    if ((t->user & 0xffff) == f->uid)
        mode = mode >> 6;
    else if (((t->user >> 16) & 0xffff) == f->gid)
        mode = mode >> 3;

    mode = mode & 0x7;

    return (mode >> type) & 0x1;
}

static inline int find_free_fd(struct task *t) {
    int fd = 0;
    for (; fd < MAXFDS; fd++)
        if (!t->fds[fd * 2])
            break;

    return fd >= MAXFDS ? -ENOMEM : fd;
}

static inline struct file_descriptor *extract_fd(struct task *t, int fd) {
    struct file_descriptor *f = (struct file_descriptor *) t->fds[fd * 2];
    return (!f || fd >= MAXFDS || fd < 0) ? NULL : f;
}

int open_at_task(struct task *t, const char *path, int flags, uint32_t mode) {
    static struct file_descriptor *fdpage = NULL;

    int fd;
    if ((fd = find_free_fd(t)) < 0)
        return fd;

    struct fs_direntry f;
    struct fs *mnt;

    ssize_t res = fs_resolve_file(path, &mnt, &f);
    if (res < 0)
        return res;

    if (flags & 0x8)
        return -ENOSUP;
    
    if (flags & O_RDONLY && !am_i_allowed(2, t, &f))
        return -EPERM;

    if (flags & O_WRONLY && !am_i_allowed(1, t, &f))
        return -EPERM;

    if (flags & 0x100 && !am_i_allowed(0, t, &f))
        return -EPERM;

    ssize_t index = -1;
    if (!fdpage) {
        fdpage = kalloc(1);
        if (!fdpage)
            return -ENOMEM;

        index = 0;
    } else {
        for (size_t i = 0; i < meminfo.pagesize / sizeof(struct file_descriptor); i++) {
            if (!fdpage[i].tasks) {
                index = i;
                break;
            }
        }

        if (index == -1) {
            uword_t *chain = (uword_t *) ((uint8_t *) fdpage + meminfo.pagesize - sizeof(uword_t));
            
            fdpage = kalloc(1);
            if (!fdpage)
                return -ENOMEM;

            *chain = (uword_t) fdpage;
        }
    }

    fdpage[index].flags = flags;
    fdpage[index].tasks = 1;
    fdpage[index].mnt = mnt;
    memcpy(&fdpage[index].ref, &f, sizeof(struct fs_direntry));
    memcpy(&fdpage[index].path, path, strlen(path) + 1);

    t->fds[fd * 2] = (uword_t) &fdpage[index];
    t->fds[fd * 2 + 1] = 0;

    return fd;
}

int open(const char *path, int flags, ...) {
    uint32_t *mode = (uint32_t *) ((uint8_t *) &flags + sizeof(int));

    return open_at_task(current_task, path, flags, flags & 0x8 ? *mode: 0);
}

off_t lseek(int fd, off_t offset, int whence) {
    struct file_descriptor *f = extract_fd(current_task, fd);
    if (!f) return -EBADF;

    uint64_t *seek = &current_task->fds[fd * 2 + 1];
    switch (whence) {
        case SEEK_SET:
            *seek = offset;
            break;
        case SEEK_CUR:
            *seek += offset;
            break;
        case SEEK_END:
            *seek = f->ref.size;
            break;
        default:
            return -EINVAL;
    }

    return *seek;
}

ssize_t read(int fd, void *restrict buf, size_t n) {
    struct file_descriptor *f = extract_fd(current_task, fd);
    if (!f) return -EBADF;

    if (!(f->flags & O_RDONLY))
        return -EPERM;

    uint64_t *seek = &current_task->fds[fd * 2 + 1];

    ssize_t result = f->mnt->driver->read(f->mnt->device, &f->ref, buf, *seek, n);
    if (result > 0) *seek += result;

    return result;
}

ssize_t write(int fd, const void *buf, size_t n) {
    struct file_descriptor *f = extract_fd(current_task, fd);
    if (!f) return -EBADF;

    if (!(f->flags & O_WRONLY))
        return -EBADF;

    uint64_t *seek_ptr = &current_task->fds[fd * 2 + 1];
    uint64_t seek = (f->flags & 0x4) ? f->ref.size : *seek_ptr; // 0x4 is append bit

    ssize_t result = f->mnt->driver->write(f->mnt->device, &f->ref, buf, seek, n);
    if (result > 0) *seek_ptr = seek + (uint64_t) result;

    return result;
}

int dup(int fd) {
    struct file_descriptor *f = extract_fd(current_task, fd);
    if (!f) return -EBADF;

    int new_fd;
    if ((new_fd = find_free_fd(current_task)) < 0)
        return new_fd;

    current_task->fds[new_fd * 2] = current_task->fds[fd * 2];
    current_task->fds[new_fd * 2] = 0;

    return new_fd;
}

int close_at_task(struct task *t, int fd) {
    struct file_descriptor *f = extract_fd(t, fd);
    if (!f) return -EBADF;

    if (f->tasks) f->tasks--;

    return 0;
}

int close(int fd) {
    return close_at_task(current_task, fd);
}