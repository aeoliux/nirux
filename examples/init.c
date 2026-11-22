extern char **environ;

long syscall(long call, ...);
int puts(const char *str);

int main(int argc, char **argv) {
    puts("init: mounting /dev/fdblk0 (0x900) as /\n");
    if ((int) syscall(0xfffb, 0x900, "fat") < 0) {
        puts("init: failed to mount /\n");
        return 1;
    }

    int pid = syscall(2);
    if (pid == 2) {
        puts("init: cloned (pid = 2)!\n");
        return 0;
    }

    puts("init: /dev/fdblk0 = / (success)\n");
    while (1) {}

    return 0;
}