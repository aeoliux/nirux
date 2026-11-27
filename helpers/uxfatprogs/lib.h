#pragma once

#include <sys/types.h>
#include <stdint.h>

typedef struct {
    uint64_t ecluster;
    uint64_t eoffset;

    uint64_t fcluster;
    uint64_t seek;
} uxfd;

extern struct fs_uxfat_superblock sblk;

uxfd *uxcreat(uxfd *parent, const char *filename, uint8_t type, mode_t mode, uid_t uid, gid_t gid);