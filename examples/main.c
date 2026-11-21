long syscall(long call, ...);

int main() {
    char hello_world_str[] = "Hello world!\n";

    return syscall(4, 1, hello_world_str, sizeof hello_world_str - 1);
}