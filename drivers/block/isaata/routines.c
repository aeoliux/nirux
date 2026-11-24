#include <ata.h>
#include <ioport.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const uint16_t iobusses[] = {ISAATA_PRIM_IO, ISAATA_SCND_IO};
static int current_drive[] = {-1, -1};

static inline int isaata_is_errored(uint8_t bus) {
    return inb(iobusses[bus] + ISAATA_REG_STATUS) & 0x1;
}

static inline int isaata_is_pio_ready(uint8_t bus) {
    return inb(iobusses[bus] + ISAATA_REG_STATUS) & 0x8;
}

static inline int isaata_is_spun_down(uint8_t bus) {
    return inb(iobusses[bus] + ISAATA_REG_STATUS) & 0x40;
}

static inline int isaata_is_busy(uint8_t bus) {
    return inb(iobusses[bus] + ISAATA_REG_STATUS) & 0x80;
}

void isaata_set_hd(uint8_t bus, uint8_t drive, uint32_t addr, uint8_t is_lba) {
    outb(iobusses[bus] + ISAATA_REG_HDDRV, ISAATAHDVAL(addr, drive, is_lba));

    current_drive[bus] = drive;
}

void isaata_lbacontrol(uint8_t bus, uint8_t drive, uint32_t lba, size_t nsec, uint8_t is_lba) {
    outb(iobusses[bus] + ISAATA_REG_LBA_LO, lba & 0xff);
    outb(iobusses[bus] + ISAATA_REG_LBA_MI, (lba >> 8) & 0xff);
    outb(iobusses[bus] + ISAATA_REG_LBA_HI, (lba >> 16) & 0xff);
    outb(iobusses[bus] + ISAATA_REG_SECCOUNT, nsec & 0xff);
    
    return isaata_set_hd(bus, drive, (lba >> 24) & 0xff, is_lba);
}

static ssize_t isaata_command(uint8_t bus, uint8_t drive, uint16_t *dst, size_t expect, uint8_t command) {
    while (isaata_is_busy(bus)) suspend();

    outb(iobusses[bus] + ISAATA_REG_CMD, command);

    ssize_t read = 0;
    while (isaata_is_busy(bus)) suspend();

    while (isaata_is_pio_ready(bus))  {
        uint16_t word = inw(iobusses[bus] + ISAATA_REG_DATA);
        if ((size_t) read < expect)
            dst[read++] = word;
    }

    if (command != ATA_IDENTIFY && isaata_is_errored(bus))
        return -1;

    return read;
}

ssize_t isaata_lba_command(uint8_t bus, uint8_t drive, uint16_t *dst, size_t expect, uint8_t command, uint32_t lba, size_t nsec) {
    while (isaata_is_busy(bus)) suspend();

    isaata_lbacontrol(bus, drive, lba, nsec, command != ATA_IDENTIFY);

    return isaata_command(bus, drive, dst, expect, command);
}

static inline void atastrrev(char *dst, size_t n) {
    for (size_t i = 0; i < n - 1; i += 2) {
        char tmp = dst[i];
        dst[i] = dst[i + 1];
        dst[i + 1] = tmp;
    }
}

int isaata_identify(uint8_t bus, uint8_t drive, struct ata_identified *restrict result) {
    uint16_t buf[256];

    if (isaata_lba_command(bus, drive, buf, 256, ATA_IDENTIFY, 0, 0) < 255)
        return -EIO;

    uint16_t *ptr = &buf[27];
    for (; *ptr != ((' ' << 8) | ' ') && ptr < &buf[27 + 20]; ptr++);
    *ptr = 0;

    memcpy(result->name, ((void *) buf) + 54, 40);
    memcpy(result->serial, ((void *) buf) + 20, 20);
    atastrrev(result->serial, 20);
    atastrrev(result->name, 40);
    result->serial[20] = 0;
    result->name[40] = 0;
    result->maxmultiple = buf[47] & 0xff;
    result->sector_size = 512; // TODO!
    result->addressing = (buf[49] & 0x200) ? 28 : 0;
    if (result->addressing) {
        result->total_lba = (buf[61] << 16) | buf[60];
    }

    return 0;
}
