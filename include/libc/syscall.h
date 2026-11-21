#pragma once

#define SYS_EXIT 1
#define SYS_READ 3
#define SYS_WRITE 4

// SPECIAL SYSCALLS

#define SYS_DREAD   0xfffd
#define SYS_DWRITE  0xfffe
#define SYS_DIOCTL  0xffff