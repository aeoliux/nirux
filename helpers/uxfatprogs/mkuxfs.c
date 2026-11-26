#include "../../include/internal/fs/uxfat.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#if defined (__linux__)
#include <linux/fs.h>
#endif

#define MiB (1024ULL * 1024ULL)
#define GiB (1024ULL * 1024ULL * 1024ULL)

int main(int argc, char **argv) {
    if (argc < 2)
        return 1;

    int fd = open(argv[1], O_WRONLY);
    if (fd == -1) {
        perror("open()");
        return 1;
    }

    uint64_t volsiz = 0;
    if (ioctl(fd, BLKGETSIZE64, &volsiz) != 0) {
        perror("ioctl BLKGETSIZE64 failed");
        close(fd);
        return 1;
    }

    // struct stat st;
    // if (fstat(fd, &st) < 0) {
    //     perror("fstat()");
    //     close(fd);
    //     return 1;
    // }

    // uint64_t volsiz = st.st_size;

    uint8_t spc = 0;
    uint8_t addr = 0;
    if (volsiz < 4084 * 512) {             // 0 - 2 MB
        addr = 12;                         // 12-bit for very small filesystems
        spc = 1;                           // 512 B clusters
    } else if (volsiz < 32 * MiB) {        // 2 MB - 32 MB
        addr = 16;
        spc  = 1;                          // 512 B clusters
    } else if (volsiz < 64 * MiB) {        // 32 MB - 64 MB
        addr = 16;
        spc  = 2;                          // 1 KB clusters
    } else if (volsiz < 128 * MiB) {       // 64 MB - 128 MB
        addr = 16;
        spc  = 4;                          // 2 KB clusters
    } else if (volsiz < 256 * MiB) {       // 128 MB - 256 MB
        addr = 16;
        spc  = 8;                          // 4 KB clusters
    } else if (volsiz < 8 * GiB) {         // 256 MB - 8 GB
        addr = 32;
        spc  = 8;                          // 4 KB clusters
    } else if (volsiz < 16 * GiB) {        // 8 GB - 16 GB
        addr = 32;
        spc  = 16;                         // 8 KB clusters
    } else if (volsiz < 32 * GiB) {        // 16 GB - 32 GB
        addr = 32;
        spc  = 32;                         // 16 KB clusters
    } else {                               // >= 32 GB
        addr = 32;
        spc  = 64;                         // 32 KB clusters
    }

    uint64_t total_sectors = volsiz / 512;
    uint64_t available = total_sectors - 1;

    uint64_t num = (available + 2ULL * spc) * addr;
    uint64_t denom = (512ULL * 8 * spc) + addr;

    uint32_t spf = (uint32_t) (num + denom - 1) / denom;

    struct fs_uxfat_superblock sblk = {
        .jmp = {0xeb, 0x3c, 0x90},
        .oem = "NXmkuxfs",
        .sector_size = 512,
        .sectors_per_cluster = spc,
        .reserved_sectors = 1,
        .fats_count = 1,
        .fat_addressing = addr & 0xf0,
        .sectors_per_fat = spf,
        .legacy_spf = 0,
        
        // legacy chs data
        .sectors_per_track = 0,
        .heads_count = 0,

        .sectors_count = total_sectors,
        .disk_no = (volsiz > 2880 * 1024) ? 0x80 : 0x00,
        .nt_flags = 0,
        .signature = 0x2A,
        .volume_serial = "0000",
        .volume_label = "uxfatvol",
        .system_id = "UXFAT   "
    };

    printf("sec: %hu, total lba: %lu, sec per fat: %u, sec per cluster: %hu, clusters: %lu\n", sblk.sector_size, sblk.sectors_count, sblk.sectors_per_fat, (uint16_t) sblk.sectors_per_cluster, (total_sectors - sblk.sectors_per_fat - 1) / sblk.sectors_per_cluster);
    printf("writing superblock...\n");

    if (write(fd, &sblk, 62) < 62) {
        perror("write(superblock)");
        close(fd);
        return 1;
    }

    printf("erasing UXFAT table...\n");
    if (lseek(fd, sblk.reserved_sectors * sblk.sector_size, SEEK_SET) < 0) {
        perror("lseek()");
        close(fd);
        return 1;
    }

    uint8_t buffer[512];
    memset(buffer, 0, 512);

    uint16_t *buf16 = (uint16_t *) buffer;
    uint32_t *buf32 = (uint32_t *) buffer;
    if (addr == 12)         *((uint16_t *) ((uint8_t *) buffer + 3)) = 0xfff << 4;
    else if (addr == 16)    buf16[2] = 0xffff;
    else if (addr == 32)    buf32[2] = 0xffffffff;

    if (write(fd, buffer, 512) < 0) {
        perror("write(fat)");
        close(fd);
        return 1;
    }

    memset(buffer, 0, 512);

    for (size_t sec = 1; sec < sblk.sectors_per_fat; sec++) {
        for (size_t i = 0; i < sblk.sector_size; i += 512) {
            if (write(fd, buffer, 512) < 0) {
                perror("write(fat)");
                close(fd);
                return 1;
            }
        }
    }

    printf("done!\n");
}