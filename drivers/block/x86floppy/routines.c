#include <kmem.h>
#include <errno.h>
#include <block.h>
#include <stdio.h>
#include <string.h>
#include <x86floppy.h>
#include <ioport.h>
#include <timer.h>

static inline int x86floppy_can_command() {
    uint8_t msr = inb(X86FLOPPY_REG_MAIN_STATUS_REGISTER);

    return (msr & 0xc0) == 0x80;
}

static inline int x86floppy_can_argument() {
    uint8_t msr = inb(X86FLOPPY_REG_MAIN_STATUS_REGISTER);

    return msr & 0x80;
}

static inline int x86floppy_should_in_fifo() {
    return (inb(X86FLOPPY_REG_MAIN_STATUS_REGISTER) & 0xc0) == 0xc0;
}

static inline int x86floppy_is_executing() {
    return inb(X86FLOPPY_REG_MAIN_STATUS_REGISTER) & 0x20;
}

static inline int x86floppy_await() {
    int timeout = 1000000;

    while (!x86floppy_irqed && timeout > 0) {
        iowait();
        timeout--;
    }

    x86floppy_irqed = 0;

    return timeout <= 0;
}

static inline void x86floppy_sleep(size_t ms) {
    size_t target = timer_elapsed + ms;
    while (timer_elapsed < target) suspend();
}

static inline uint8_t x86floppy_decode_drive_id(uint32_t drive) {
    return (drive & 0xff0000) >> 16;
}

uint8_t x86floppy_dor = 0x0;
static inline void x86floppy_update_dor(uint8_t dor) {
    outb(X86FLOPPY_REG_DIGITAL_OUTPUT_REGISTER, dor);
    x86floppy_dor = dor;
}

uint8_t x86floppy_version = 0;
uint8_t x86floppy_fifo[16] = {0};
uint8_t *x86floppy_dma = NULL;
uint8_t x86floppy_nread = 0;
uword_t x86floppy_flush_buffer() {
    // puts("// x86floppy flushing buffer - ");

    while (x86floppy_should_in_fifo() && x86floppy_nread < 16) {
        uint8_t byte = inb(X86FLOPPY_REG_DATA_FIFO);
        // printf("%x ", (uint32_t) byte);

        x86floppy_fifo[x86floppy_nread++] = byte;
    }
    // putchar('\n');

    return 0;
}

int x86floppy_configure() {
    uint8_t param = (1 << 5) | (1 << 4) | 15;
    int result = x86floppy_perform_command(X86FLOPPY_CMD_CONFIGURE, 0x0, param, 0x0, X86FLOPPY_CMD_EOA);
    if (!result) {
        x86floppy_perform_command(X86FLOPPY_CMD_LOCK, X86FLOPPY_CMD_EOA);

        if (!x86floppy_nread) {
            printf("// x86floppy configuration lock failed\n");
            return -1;
        }
        x86floppy_nread--;

        return 0;
    }

    printf("// x86floppy controller configuration failure\n");

    return result;
}

int x86floppy_reset() {
    x86floppy_irqed = 0;
    x86floppy_nread = 0;

    x86floppy_update_dor(0x0);
    for (size_t i = 0; i < 20; i++) iowait();
    x86floppy_update_dor(0xc);
    for (size_t i = 0; i < 20; i++) iowait();

    if (x86floppy_await()) return -0xff;

    for (int i = 0; i < 4; i++) {
        x86floppy_nread = 0;
        x86floppy_perform_command(X86FLOPPY_CMD_SENSE_INTERRUPT, X86FLOPPY_CMD_EOA);
        if (x86floppy_nread > 0 && x86floppy_fifo[0] == 0x80)
            break;
    }

    memset(x86floppy_fifo, 0, 16);
    x86floppy_nread = 0;

    return 0;
}

int x86floppy_select_drive(uint32_t drive) {
    uint32_t datarate = (drive & BLOCK_FLOPPY_3ED) ? 1000000 : 500000;

    outb(X86FLOPPY_REG_CONFIGURATION_CONTROL_REGISTER, (drive & BLOCK_FLOPPY_3ED) ? 3 : 0);

    x86floppy_update_dor((x86floppy_dor & ~0x3) | (x86floppy_decode_drive_id(drive) & 0x3));

    uint32_t srt = 16 - (8 * datarate / 500000);
    uint32_t hlt = 10 * datarate / 1000000;
    uint32_t hut = 24 * datarate / 8000000;

    hlt = hlt ? 1 : hlt;

    x86floppy_perform_command(X86FLOPPY_CMD_SPECIFY, srt << 4 | hut, hlt << 1, X86FLOPPY_CMD_EOA);

    return 0;
}

uint32_t x86floppy_current_drive = 0;
void x86floppy_set_motor(uint32_t drive_id, uint8_t state) {
    if (x86floppy_current_drive == drive_id && state)
        return;

    if (state)
        x86floppy_select_drive(drive_id);

    uint32_t drive = x86floppy_decode_drive_id(drive_id);

    uint8_t mask = 1 << (drive + 4);
    x86floppy_update_dor(state ? (x86floppy_dor | mask) : (x86floppy_dor & ~mask));

    if (state) {
        if (x86floppy_current_drive && drive_id != x86floppy_current_drive)
            x86floppy_set_motor(x86floppy_current_drive, 0);

        x86floppy_sleep(500);
        x86floppy_current_drive = drive_id;
    } else {
        x86floppy_current_drive = 0;
    }
}

int x86floppy_set_dma(uint8_t is_write) {
    // mask 2 channel
    outb(0xa, 0x4 | 0x2);

    // reset flip-flop switch and save address offset
    outb(0xc, 0xff);
    outb(0x4, (uword_t) x86floppy_dma & 0xff);
    outb(0x4, ((uword_t) x86floppy_dma >> 8) & 0xff);

    // reset flip-flop switch and store buffer size
    outb(0xc, 0xff);
    outb(0x5, 511 & 0xff);
    outb(0x5, (511 >> 8) & 0xff);

    // store memory segment of buffer size
    outb(0x81, ((uword_t) x86floppy_dma >> 16) & 0xff);

    outb(0xb, is_write ? 0x4a : 0x46);

    // unmask 2 channel
    outb(0xa, 0x2);

    return 0;
}

size_t x86floppy_sense() {
    x86floppy_nread = 0;
    x86floppy_perform_command(
        X86FLOPPY_CMD_SENSE_INTERRUPT,
        X86FLOPPY_CMD_EOA
    );

    if (x86floppy_nread > 1)
        x86floppy_nread = 1;

    return (x86floppy_nread > 0)
        ? (x86floppy_fifo[--x86floppy_nread] & 0xe0) != 0x20
        : 0xff;
}

int x86floppy_recalibrate() {
    x86floppy_irqed = 0;
    x86floppy_perform_command(
        X86FLOPPY_CMD_RECALIBRATE,
        x86floppy_decode_drive_id(x86floppy_current_drive),
        X86FLOPPY_CMD_EOA
    );
    if (x86floppy_await()) return -1;

    return x86floppy_sense();
}

int x86floppy_init(uint32_t *drives, size_t n) {
    // reset floppy controller
    int reset_result = x86floppy_reset();
    if (reset_result) return reset_result;

    // send VERSION command
    x86floppy_perform_command(X86FLOPPY_CMD_VERSION, X86FLOPPY_CMD_EOA);
    // if no response, don't continue
    if (!x86floppy_nread) return -0xfe;

    // store floppy controller version
    x86floppy_version = x86floppy_fifo[--x86floppy_nread];

    switch (x86floppy_version) {
        // 0x90 - new generation
        case 0x90: if (x86floppy_configure()) return -0xfd; else break;
        // 0x80 - invalid command means legacy controller
        case 0x80: puts("// x86floppy - legacy PC/AT controller detected\n");
        default: printf("// x86floppy init failed - unknown controller version: %x\n", x86floppy_version); return -0xfe;
    }

    // allocate dma buffer
    x86floppy_dma = kalloc(1);
    if (!x86floppy_dma) {
        puts("// x86floppy dma buffer alloc, kalloc(1) returned 0\n");
        return -ENOMEM;
    }

    if ((uword_t) x86floppy_dma > 0xffffff) {
        kfree(x86floppy_dma);
        puts("// x86floppy couldn't allocate DMA buffer below 16MB mark\n");
        return -ENOMEM;
    }

    for (size_t i = 0; i < n; i++) {
        x86floppy_set_motor(drives[i], 1);
        int result = x86floppy_recalibrate();
        x86floppy_set_motor(drives[i], 0);

        if (result) {
            printf("// x86floppy drive %u recalibration failed! Disabling drive\n", i);
            drives[i] = BLOCK_FLOPPY_NULL;
        }
    }

    return x86floppy_version;
}

uint8_t x86floppy_seeked_head = 0, x86floppy_seeked_cylinder = 0, x86floppy_seeked_drive = 0;
int x86floppy_seek(uint8_t head, uint8_t cylinder) {
    if (head == x86floppy_seeked_head && cylinder == x86floppy_seeked_cylinder && x86floppy_seeked_drive == x86floppy_current_drive)
        return 0;

    x86floppy_irqed = 0;
    x86floppy_perform_command(
        X86FLOPPY_CMD_SEEK,
        (head << 2) | x86floppy_decode_drive_id(x86floppy_current_drive),
        cylinder,
        X86FLOPPY_CMD_EOA
    );
    if (x86floppy_await()) return -1;

    int ret = x86floppy_sense();
    if (!ret) {
        x86floppy_seeked_head = head;
        x86floppy_seeked_cylinder = cylinder;
        x86floppy_seeked_drive = x86floppy_current_drive;
    }

    return ret;
}

int x86floppy_perform_dma(uint8_t is_write, uint8_t spt, uint8_t cylinder, uint8_t head, uint8_t sec) {
    if (x86floppy_seek(head, cylinder)) return -5;

    x86floppy_set_dma(is_write);

    x86floppy_irqed = 0;
    x86floppy_nread = 0;
    int result = x86floppy_perform_command(
        0x40 | (is_write ? X86FLOPPY_CMD_WRITE_DATA : X86FLOPPY_CMD_READ_DATA),
        head << 2 | x86floppy_decode_drive_id(x86floppy_current_drive),
        cylinder,
        head,
        sec,
        2,
        spt,
        0x1b,
        0xff,
        X86FLOPPY_CMD_EOA
    );
    if (result) return result;

    if (x86floppy_await()) return -2;

//     puts("// x86floppy dma response bytes: ");
//     for (size_t i = 0; i < x86floppy_nread; i++)
//         printf("%x ", x86floppy_fifo[i]);
//     putchar('\n');

    result = (x86floppy_nread > 0)
        ? -(x86floppy_fifo[0] & 0xc0)
        : -3;

    x86floppy_nread = 0;

    return result;
}

int x86floppy_perform_command(uint8_t command, ...) {
    uint16_t *arg = (uint16_t *) ((uint8_t *) &command) + 2;

    x86floppy_irqed = 0;

    int timeout = 10000;
    while (!x86floppy_can_command() && timeout > 0) {
        if (x86floppy_should_in_fifo()) inb(X86FLOPPY_REG_DATA_FIFO);

        iowait();
        timeout--;
    }

    if (timeout <= 0) return -1;

    outb(X86FLOPPY_REG_DATA_FIFO, command);

    // printf("// x86floppy command %x args: ", (uint32_t) command);
    while (*arg != X86FLOPPY_CMD_EOA) {
        timeout = 10000;
        while (!x86floppy_should_in_fifo() && !x86floppy_can_argument() && timeout > 0) {
            iowait();
            timeout--;
        }

        if (x86floppy_should_in_fifo()) {
            printf("// x86floppy error code: %x\n", x86floppy_fifo[--x86floppy_nread]);
            return -2;
        }

        if (timeout <= 0) return -2;

        outb(X86FLOPPY_REG_DATA_FIFO, (uint8_t) (*arg & 0xff));

//        printf("%x ", (uint32_t) (uint8_t) (*arg & 0xff));
        arg += 2;
    }
//    putchar('\n');

    for (size_t i = 0; i < 1000; i++) {
        if (x86floppy_can_argument()) break;
        iowait();
    }

    if (!x86floppy_is_executing() && x86floppy_should_in_fifo())
        x86floppy_flush_buffer();

    return 0;
}
