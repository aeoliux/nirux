#include <ata.h>
#include <ioport.h>
#include <stdio.h>
#include <driver.h>
#include <modctl.h>
#include <block.h>
#include <device.h>

uint8_t prim_bus = 0, scnd_bus = 0;

int isaata_probe() {
    prim_bus = inb(ISAATA_PRIM_IO + ISAATA_REG_STATUS) != 0xff;
    scnd_bus = inb(ISAATA_SCND_IO + ISAATA_REG_STATUS) != 0xff;

    if (prim_bus || scnd_bus) {
        printf("// isaata detected ATA busses (n = %d)\n", (int)(prim_bus + scnd_bus));

        struct ata_identified dev;
        for (size_t bus = 0; bus < (prim_bus + scnd_bus); bus++) {
            for (size_t drive = 0; drive < 2; drive++) {
                int n = isaata_identify(bus, drive, &dev);
                if (n < 0)
                    continue;

                printf("// isaata (bus: %u, drive: %u) - %uB * %uLBA%u, name = %s\n", bus, drive, (uint32_t) dev.sector_size, (uint32_t) dev.total_lba, (uint32_t) dev.addressing, dev.name);
            }
        }
    }

    return 0;
}

struct nirux_block_driver block_isaata = {
    .name = "isaata",

    .probe = isaata_probe,
    .read = NULL,
    .write = NULL
};