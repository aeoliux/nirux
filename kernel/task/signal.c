#include <stdio.h>
#include <task.h>
#include <errno.h>
#include <signal.h>
#include <ioport.h>

#define ABORTINFO(str) printf("// PID: %d - " str "\n", t->pid);

void task_sig_default(struct task *t, int sig) {
    if (t->is_kthread) return;
    int secure = t->pid == 1;

    switch (sig) {
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
        case SIGKILL:
        case SIGTERM:
            if (!secure) {
                ABORTINFO("killed");
                task_exit(t, TASK_KILLED, sig);
            }
            return;
        
        case SIGILL:
            ABORTINFO("illegal instruction");
            goto abort;
        case SIGABRT:
            ABORTINFO("aborted");
            goto abort;
        case SIGBUS:
            ABORTINFO("bus error");
            goto abort;
        case SIGFPE:
            ABORTINFO("fpu exception");
            goto abort;
        case SIGSEGV:
            ABORTINFO("segmentation fault");
            goto abort;
        case SIGPIPE:
            ABORTINFO("broken pipe");
            goto abort;
        case SIGSTKFLT:
            ABORTINFO("stack fault");
            goto abort;
        case SIGURG:
            ABORTINFO("cpu time limit exceeded");
            goto abort;
        case SIGPWR:
            ABORTINFO("power failure");
            goto abort;
        case SIGSYS:
            ABORTINFO("bad system call");
            goto abort;

        case SIGSTOP:
            t->state = TASK_PAUSED;
            return;
        case SIGCONT:
            t->state = TASK_RUNNING;
            return;
    }

    abort:
        task_exit(t, TASK_FAULT, sig);
        return;
}

void task_sig_dispatch(struct task *t) {
    if (t->is_kthread || !t->sig_pending) return;

    int sig_id = __builtin_ctz(t->sig_pending);
    t->sig_pending &= ~(1 << sig_id);

    int sig = sig_id + 1;

    sighandler_t h = t->sig_handlers[sig_id];
    if (h == SIG_DFL || sig == SIGKILL)
        return task_sig_default(t, sig);
    if (h == SIG_IGN)
        return;

    // TODO! CUSTOM SIGNAL HANDLERS
    task_exit(t, TASK_FAULT, 0xff);
}

int task_sig_send(struct task *t, int sig) {    
    if (sig < 1 || sig > SIGCOUNT)
        return -EINVAL;

    t->sig_pending |= 1 << (sig - 1);

    if (!(t->state & TASK_FINISHED)) {
        t->state = TASK_RUNNING;
        t->substate = 0;
    }

    return 0;
}

int task_sig_send_self(int sig) {
    if (!current_task->is_kthread) {
        task_sig_send(current_task, sig);

        ienable();
        while (1) suspend();
    }

    return -EPERM;
}

int kill(pid_t pid, int sig) {
    int err = -ESRCH;
    struct task *t;
    if (pid > 0) {
        if (!(t = pidtable_task(pid)))
            return -ESRCH;

        if ((current_task->euser & 0xffff) && ((current_task->euser & 0xffff) != (t->euser & 0xffff)))
            return -EPERM;

        return sig ? task_sig_send(t, sig) : 0;
    } else if (!pid) {
        for (pid = 1; pid < last_pid; pid++) {
            if (current_task->pid != pid && (t = pidtable_task(pid)) && t->pgid == current_task->pgid) {
                err = kill(pid, sig);
                if (err < 0)
                    return err;
            }
        }
    } else if (pid == -1) {
        for (pid = 2; pid < last_pid; pid++) {
            if (current_task->pid != pid) {
                err = kill(pid, sig);
                if (err == -EPERM)
                    err = 0;
                else if (err < 0)
                    return err;
            }
        }
    } else {
        pid_t pgid = -pid;
        for (pid = 1; pid < last_pid; pid++) {
            if (current_task->pid != pid && (t = pidtable_task(pid)) && t->pgid == pgid) {
                err = kill(pid, sig);
                if (err < 0)
                    return err;
            }
        }
    }

    return err;
}