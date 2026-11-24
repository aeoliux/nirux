#include <sys/wait.h>
#include <panic.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <kmem.h>
#include <errno.h>
#include <ioport.h>

size_t max_pids;
pid_t last_pid;
size_t pidtab_pages;
pid_t nextpid;
struct task **pidtab;

void pidtable_init() {
    void *ret = __builtin_return_address(0);

    last_pid = 0;
    nextpid = 1;
    max_pids = 32768;
    pidtab_pages = max_pids * sizeof(uword_t) / meminfo.pagesize;

    pidtab = kalloc(pidtab_pages);
    if (!pidtab)
        panic("not enough memory for pidtable", ret);

    memset(pidtab, 0, max_pids * sizeof(uword_t));
    pidtab[0] = &ktask;
}

pid_t pidtable_register(struct task *t) {
    int critical_search = 0, nextpid_is_init = 0;

    if (t->is_kthread && nextpid == 1) {
        nextpid_is_init = 1;
        nextpid++;
    }

    if (nextpid >= max_pids) {
        nextpid = 2;
        critical_search = 1;
    }

    while (pidtab[nextpid] && nextpid < max_pids)
        nextpid++;

    if (nextpid >= max_pids) {
        if (critical_search) return -EAGAIN;
        return pidtable_register(t);
    }

    t->pid = nextpid;
    pidtab[nextpid++] = t;

    if (nextpid_is_init) nextpid = 1;

    if (last_pid < t->pid)
        last_pid = t->pid;

    return t->pid;
}

struct task *pidtable_task(pid_t pid) {
    return (last_pid >= pid) ? pidtab[pid] : NULL;
}

pid_t fork() {
    if (current_task->childs_count >= MAX_CHILDS)
        return -ENOMEM;

    pid_t ret;
    if ((ret = task_clone(current_task)) < 0)
        return ret;

    current_task->childs[current_task->childs_count++] = ret;

    return ret;
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {
    struct task *t = NULL;

    do {
        for (size_t i = 0; i < current_task->childs_count; i++) {
            if (current_task->childs[i] <= 0) continue;

            struct task *c = pidtable_task(current_task->childs[i]);
            if (
                c && c->parent == current_task->pid &&
                (((pid < -1) && (c->pgid == -pid)) ||
                (pid == -1) ||
                ((!pid) && (c->pgid == current_task->pgid)) ||
                ((pid > 0) && (c->pid == pid)))
            ) {
                if (c->state & TASK_FINISHED) {
                    t = c;
                    break;
                } else if (c->state & TASK_RUNNING && options & WNOHANG) {
                    return 0;
                }
            }
        }

        if (!(options & WNOHANG)) suspend();
    } while (!(options & WNOHANG) && !t);

    if (t) {
        *wstatus = 0;
        if (t->state == TASK_EXITED) *wstatus |= (1 << 8);
        *wstatus |= (t->substate & 0xff);

        pid = t->pid;
        task_release_mem(t);
        kfreen(t, 2);

        return pid;
    }

    return -ECHILD;
}