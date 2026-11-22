extern char **environ;

long syscall(long call, ...);
int puts(const char *str);

int main(int argc, char **argv) {
    puts("init: mounting /dev/fdblk0 (0x900) as /\n");
    if ((int) syscall(0xfffb, 0x900, "fat") < 0) {
        puts("init: failed to mount /\n");
        return 1;
    }

    puts("init: /dev/fdblk0 = / (success)\n");

    return 0;
}