#include "../../include/internal/fs/uxfat.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include "lib.h"

struct fs_uxfat_superblock sblk;
int fd, base;

uint8_t fat_buf[512], clus_buf[512];
uint64_t fat_buffered = 0, clus_sec_buffered = 0;
uint64_t end_clus = 0, bad_clus = 0;

int volopen(int flags) {
    base = open("/tmp/uxmount", O_RDONLY);
    if (base < 0) {
        perror("open(base)");
        return -1;
    }

    if (read(base, &sblk, 62) < 62) {
        perror("read(base, sblk)");
        close(base);
        return -1;
    }

    switch (sblk.fat_addressing) {
        case 12: end_clus = 0xfff; bad_clus = 0xffe; break;
        case 16: end_clus = 0xffff; bad_clus = 0xfffe; break;
        case 32: end_clus = 0xffffffff; bad_clus = 0xfffffffe; break;
        default: printf("invalid fat addressing\n"); close(base); return -1;
    }

    char path[PATH_MAX];
    if (read(base, path, PATH_MAX) <= 0) {
        perror("read(base, path)");
        close(base);
        return -1;
    }

    fd = open(path, flags);
    if (fd < 0) {
        close(base);
        perror("open(volume)");
    }
    return fd;
}

int volclose() {
    pwrite(base, &sblk, 62, 0);
    pwrite(fd, &sblk, 62, 0);

    close(fd);
    close(base);
    return 0;
}

static void fat_offsets(uint64_t cluster, uint64_t *fat_sector, uint64_t *ent_off) {
    uint64_t fat_off;
    switch (sblk.fat_addressing) {
        case 12: fat_off = cluster + (cluster / 2); break;
        case 16: fat_off = cluster * 2; break;
        default: fat_off = cluster * 4; break;
    }

    *fat_sector = sblk.reserved_sectors + (fat_off / sblk.sector_size);
    *ent_off = fat_off % sblk.sector_size;
}

static int64_t read_fat_sector(uint64_t fat_sector) {
    if (!fat_buffered || fat_buffered == fat_sector) {
        if (pread(fd, fat_buf, sblk.sector_size, fat_sector * sblk.sector_size) < 512) {
            perror("pread(fat)");
            return -1;
        }

        fat_buffered = fat_sector;
    }

    return 0;
}

static int64_t read_fat(uint64_t cluster) {
    uint64_t fat_sector, ent_off;
    fat_offsets(cluster, &fat_sector, &ent_off);

    if (read_fat_sector(fat_sector)) return -1;

    switch (sblk.fat_addressing) {
        case 12: return (cluster & 1) ? (*(uint16_t *) &fat_buf[ent_off]) >> 4 : (*(uint16_t *) &fat_buf[ent_off]) & 0xfff;
        case 16: return (*(uint16_t *) &fat_buf[ent_off]);
        default: return (*(uint32_t *) &fat_buf[ent_off]);
    }
}

static int set_fat(uint64_t cluster, uint64_t new_val) {
    uint64_t fat_sector, ent_off;
    fat_offsets(cluster, &fat_sector, &ent_off);

    if (read_fat_sector(fat_sector)) return -1;

    uint16_t curr;
    switch (sblk.fat_addressing) {
        case 12:
            curr = (*(uint16_t *) &fat_buf[ent_off]);
            if (cluster & 1) {
                curr &= ~(0xfff << 4);
                curr |= new_val << 4;
            } else {
                curr &= ~0xfff;
                curr |= new_val;
            }
            (*(uint16_t *) &fat_buf[ent_off]) = curr;
            break;

        case 16: (*(uint16_t *) &fat_buf[ent_off]) = new_val & 0xffff; break;
        default: (*(uint32_t *) &fat_buf[ent_off]) = new_val & 0xffffffff; break;
    }

    if (pwrite(fd, fat_buf, 512, fat_sector * sblk.sector_size) < sblk.sector_size) {
        perror("pwrite(fat)");
        return -1;
    }

    return 0;
}

static int64_t next_free_cluster() {
    uint64_t start = sblk.seek;

    if (!start) start = 3;
    
    uint64_t nclus = ((uint64_t) sblk.sectors_count - 1UL) / sblk.sectors_per_cluster;
    for (uint64_t clus = start; clus < nclus; clus++)
        if (!read_fat(clus))
            return clus;

    return 0;
}

static int read_clus(uint64_t cluster, size_t sec_off) {
    uint64_t sector = sblk.reserved_sectors + sblk.sectors_per_fat + ((cluster - 2) * sblk.sectors_per_cluster) + sec_off;

    if (!clus_sec_buffered || clus_sec_buffered != sector) {
        if (pread(fd, clus_buf, sblk.sector_size, sector * sblk.sector_size) < sblk.sector_size) {
            perror("pread(clus)");
            return -1;
        }

        clus_sec_buffered = sector;
    }
    return 0;
}

int64_t seek_clusters(
    size_t cluster,
    size_t seek_to_byte
) {
    int64_t clus = cluster;
    size_t seeked = 0;
    while (seek_to_byte / (sblk.sector_size * sblk.sectors_per_cluster) > seeked) {
        clus = read_fat(clus);

        if (clus < 0)
            return clus;

        if (clus == end_clus)
            return 0;

        if (clus == 0) {
            printf("corrupted fat filesystem\n");
            return -1;
        }

        seeked++;
    }

    return clus;
}

static int64_t get_entry(uint64_t cluster, struct fs_uxfat_direntry *en, uint64_t *seek) {
    cluster = seek_clusters(cluster, (*seek) * 32);
    if (cluster <= 0) return cluster;

    size_t offset_in_clus = (*seek * 32) % (sblk.sector_size * sblk.sectors_per_cluster);
        
    uint64_t offset = offset_in_clus % sblk.sector_size;
    uint64_t sector = (*seek) * 32 / sblk.sector_size;

    if (read_clus(cluster, sector))
        return -1;

    memcpy(en, clus_buf + offset, 32);

    return cluster;
}

uxfd *uxcreat(uxfd *parent, const char *filename, uint8_t type, mode_t mode, uid_t uid, gid_t gid) {
    struct fs_uxfat_direntry dir;
    uint64_t seek = 0;
    int64_t cluster = parent ? parent->fcluster : 2;

    int64_t next;
    while ((next = get_entry(cluster, &dir, &seek)) >= 0) {
        if (!next) {
            uint64_t n;
            if (!(n = next_free_cluster()))
                return NULL;

            set_fat(cluster, n);
            set_fat(n, end_clus);
            sblk.seek = n + 1;
            continue;
        }

        if (!(dir.attr & UXFAT_SET)) {
            
        }

        seek++;

        cluster = next;
    }
}

uxfd *uxopen(char *path) {
    uxfd *f = NULL, *parent = NULL, tmp = {0, 0, 0};

    char *filename = path;
    for (size_t i = strlen(path) - 1; i >= 0; i--) {
        if (i && path[i] == '/') {
            if (i > 0) path[i] = 0;
            filename = &path[i + 1];

            parent = uxopen(path);
            if (!parent)
                return NULL;
        }
    }

    f = malloc(sizeof(uxfd));
    if (!f) return NULL;

    if (!parent) {
        f->ecluster = 2;
    }
}
