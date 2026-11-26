#include <irq.h>
#include <string.h>
#include <unistd.h>
#include <ata.h>
#include <ioport.h>
#include <stdio.h>
#include <driver.h>
#include <modctl.h>
#include <block.h>
#include <device.h>

uint8_t isaata_prim_bus = 0, isaata_scnd_bus = 0;
uint8_t isaata_busses_count = 0;

int isaata_probe() {
    printf("// isaata initialising legacy PIO mode driver for ATA devices\n");

    isaata_prim_bus = inb(ISAATA_PRIM_IO + ISAATA_REG_STATUS) != 0xff;
    isaata_scnd_bus = inb(ISAATA_SCND_IO + ISAATA_REG_STATUS) != 0xff;

    if (isaata_prim_bus || isaata_scnd_bus) {
        isaata_busses_count = (int)(isaata_prim_bus + isaata_scnd_bus);
        isaata_prim_bus = 0;
        isaata_scnd_bus = 0;

        printf("// isaata detected ATA busses (n = %d)\n", (int) isaata_busses_count);

        struct ata_identified dev;
        struct mbr mbr;
        size_t irqs = 0;
        for (size_t bus = 0; bus < isaata_busses_count; bus++) {
            if (irqs <= bus) {
                register_irq(32 + 14 + bus, isaata_irq[bus], 0);
                set_irq_state(32 + 14 + bus, 1);
                irqs++;
            }

            for (size_t drive = 0; drive < 2; drive++) {
                int n = isaata_identify(bus, drive, &dev);
                if (n < 0)
                    continue;

                printf("// isaata (bus: %u, drive: %u) ", bus, drive);
                switch (dev.type) {
                    case BLOCK_DRIVE:
                        printf("- %uB * %uLBA%u, name = %s\n", (uint32_t) dev.sector_size, (uint32_t) dev.total_lba, (uint32_t) dev.addressing, dev.name);
                        break;
                    case BLOCK_OPTICAL:
                        printf("- ATAPI optical drive, name = %s\n", dev.name);
                        break;
                    case BLOCK_UNKNOWN:
                        printf("- ATAPI unknown drive, name = %s\n", dev.name);
                        break;
                }

                void *reserved = (void *) (dev.maxmultiple << 8 | dev.addressing);

                struct nirux_block_device blk = {
                    .addr = 8 * 256 + 128 + bus * 2 + drive,
                    .type = ((drive << 9) | (bus << 8) | dev.type) & ~BLOCK_PARTITION,
                    .removable = dev.type != BLOCK_DRIVE,
                    .sector_size = dev.sector_size,
                    .total_lba = dev.total_lba,

                    .driver = &block_isaata,
                    .reserved = reserved
                };

                if (block_interface.modctl(BLOCK_MODCTL_REGISTER, &blk) < 0) {
                    if (!bus) isaata_prim_bus++;
                    else isaata_scnd_bus++;
                }
            }
        }
    }

    return 0;
}

ssize_t isaata_read(struct nirux_block_device *blk, size_t seek, void *restrict buf, size_t n) {
    uint8_t bus = (blk->type >> 8) & 0x1;
    uint8_t drive = (blk->type >> 9) & 0x1;

    uint8_t addressing = (uint8_t) ((uword_t) blk->reserved & 0xff);

    return isaata_read_sectors(bus, drive, buf, seek, n, addressing);
}

struct nirux_block_driver block_isaata = {
    .name = "isaata",

    .probe = isaata_probe,
    .read = isaata_read,
    .write = NULL
};