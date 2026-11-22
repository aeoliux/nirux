#include <panic.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <kmem.h>
#include <errno.h>

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
    int critical_search = 0;

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

    if (last_pid < t->pid)
        last_pid = t->pid;

    return t->pid;
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {

}

pid_t fork() {
    if (current_task->childs_count >= MAX_CHILDS)
        return -ENOMEM;

    pid_t ret;
    if ((ret = task_clone(current_task)) < 0)
        return ret;

    current_task->childs[current_task->childs_count++] = ret;

    return 0;
}