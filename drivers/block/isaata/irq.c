#include <ioport.h>
#include <irq.h>
#include <ata.h>
#include <stdio.h>

static const uint16_t iobusses[] = {ISAATA_PRIM_IO, ISAATA_SCND_IO};
size_t isaata_expect[] = {0, 0};
uint16_t *isaata_buffers[] = {NULL, NULL};
uint16_t isaata_status[] = {0, 0};

void isaata_common_irq(uint8_t bus) {
    uint8_t st = inb(iobusses[bus] + ISAATA_REG_STATUS);

    if (st & 0x21) {
        isaata_status[bus] = inb(iobusses[bus] + ISAATA_REG_ERR);
        isaata_buffers[bus] = NULL;
        return;
    }

    isaata_status[bus] = 0;

    if (!(st & 0x8))
        return;

    size_t n = 0;
    size_t expect = 256;
    while (n < 256) {
        uint16_t word = inw(iobusses[bus] + ISAATA_REG_DATA);

        if (isaata_buffers[bus] && isaata_expect[bus]) {
            *isaata_buffers[bus]++ = word;
            isaata_expect[bus]--;
        }

        n++;
    }

    if (!isaata_expect[bus]) isaata_buffers[bus] = NULL;
}

uword_t isaata_prim_irq(struct handler_args *args) {
    isaata_common_irq(0);
    return 0;
}

uword_t isaata_scnd_irq(struct handler_args *args) {
    isaata_common_irq(1);
    return 0;
}

uword_t (*isaata_irq[])(struct handler_args *) = {
    isaata_prim_irq,
    isaata_scnd_irq
};