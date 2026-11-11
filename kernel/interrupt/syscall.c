#include <irq.h>
#include <stdio.h>

uword_t syscall(struct handler_args *args) {
    switch (args->arg0) {
        case 1:
            puts((const char *) args->arg1);
            break;
    }

    return 0;
}