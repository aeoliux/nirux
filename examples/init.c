extern char **environ;

long syscall(long call, ...);
int puts(const char *str);
char *i32toa(char *dst, unsigned int n, unsigned char base, unsigned char unsig);

int main(int argc, char **argv) {
    char buf[16];
    puts("init: mounting /dev/fdblk0 (0x900) as /\n");
    if ((int) syscall(0xfffb, 0x900, "fat") < 0) {
        puts("init: failed to mount /\n");
        return 1;
    }

    int pid = syscall(2);
    if (!pid) {
        puts("init: cloned (pid = 2)!\n");

        for (int i = 0; i < 500000; i++);

        puts("init: cloned (pid = 2)!\n");

        // syscall(1, 0);
        while (1) {}
    }

    if (pid == 2) puts("init: child spawned, pid = 2\n");

    for (int i = 0; i < 2000000; i++);

    if (!syscall(37, pid, 1)) puts("init: forked process killed\n");

    puts("init: /dev/fdblk0 = / (success)\n");

    while (1) {}

    return 0;
}