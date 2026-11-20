#include <irq.h>
#include <task.h>

uword_t syscall(struct handler_args *args) {
    switch (args->arg0) {
        case 0:
            task_exit(args->caller, TASK_EXITED, (uint64_t) args->arg1);
            break;
    }

    return 0;
}
