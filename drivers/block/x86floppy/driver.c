#include <irq.h>
#include <block.h>
#include <ioport.h>
#include <stdio.h>
#include <x86floppy.h>

static inline uint32_t x86floppy_convert_result(uint32_t result) {
    switch (result) {
        case 0: return BLOCK_FLOPPY_NULL;
        case 1: return BLOCK_FLOPPY_5DD;
        case 2: return BLOCK_FLOPPY_5HD;
        case 3: return BLOCK_FLOPPY_3DD;
        case 4: return BLOCK_FLOPPY_3HD;
        case 5: return BLOCK_FLOPPY_3ED;
    }

    return BLOCK_FLOPPY_NULL;
}

ssize_t x86floppy_probe(uint32_t *out, size_t limit) {
    outb(0x70, (1 << 7) | 0x10);
    uint8_t val = inb(0x71);

    uint8_t master = val >> 4;
    uint8_t slave = val & 0xf;

    size_t n = 0;
    if (limit >= 1) {
        out[0] = x86floppy_convert_result(master);
        if (out[0]) n++;
    }

    if (limit >= 2) {
        out[1] = x86floppy_convert_result(slave);
        if (out[1]) n++;
    }

    if (n > 0) {
        register_irq(0x20 + 0x6, x86floppy_irq6, 0);
        set_irq_state(0x20 + 0x6, 1);

        int version = x86floppy_init(out, n);
        if (version < 0) {
            printf("// x86floppy probe failed: controller reset result = %d\n", version);
            return 0;
        }

        printf("// x86floppy controller version code: %x\n", version);
        puts("// x86floppy drives: ");
        for (size_t i = 0; i < n; i++) {
            char *drive_info = "";
            switch (out[i]) {
                case BLOCK_FLOPPY_5DD: drive_info = "5.25\" 360KB"; break;
                case BLOCK_FLOPPY_5HD: drive_info = "5.25\" 1.2MB"; break;
                case BLOCK_FLOPPY_3DD: drive_info = "3.5\" 720KB"; break;
                case BLOCK_FLOPPY_3HD: drive_info = "3.5\" 1.44MB"; break;
                case BLOCK_FLOPPY_3ED: drive_info = "3.5\" 2.88MB"; break;
            }

            printf("%u = %s", i, drive_info);
            if (i < n - 1) puts(", ");
        }

        putchar('\n');
    }

    return n;
}

struct nirux_block_floppy_driver x86floppy_driver = {
    .probe = x86floppy_probe
};