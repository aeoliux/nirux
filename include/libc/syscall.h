#pragma once

#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_WAITPID     7

#define SYS_EXECVE      11

#define SYS_LSEEK       19
#define SYS_GETPID      20
#define SYS_MOUNT       21
#define SYS_UMOUNT      22

#define SYS_KILL        37

#define SYS_DUP         41

#define SYS_GETEUID     49
#define SYS_GETEGID     50

#define SYS_IOCTL       54

// NIRUX EXCLUSIVE

#define SYS_FMOUNT 0x1000

// SPECIAL SYSCALLS

#define SYS_DMNTRT  0xfffb
#define SYS_DMOUNT  0xfffc
#define SYS_DREAD   0xfffd
#define SYS_DWRITE  0xfffe
#define SYS_DIOCTL  0xffff