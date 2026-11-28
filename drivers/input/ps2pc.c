#include <modctl.h>
#include <stdint.h>
#include <input.h>
#include <device.h>
#include <ioport.h>
#include <errno.h>
#include <stdio.h>
#include <irq.h>
#include <kmem.h>
#include <string.h>

uint8_t ps2pc_dualport = 0;
uint8_t ps2pc_port0 = 0;
uint8_t ps2pc_port1 = 0;

size_t ps2pc_kbfifo_i = 0, ps2pc_auxfifo_i = 0;
uint8_t *ps2pc_kbfifo = NULL, *ps2pc_auxfifo = NULL;

static inline int is_input_full() {
    return inb(0x64) & 0x2;
}

static inline int is_output_ready() {
    return inb(0x64) & 0x1;
}

static inline int is_for_controller() {
    return inb(0x64) & 0x8;
}

static inline int await_for_input() {
    int timeout = 1000;
    while (is_input_full() && timeout > 0) {
        timeout--;
        iowait();
    }

    return timeout <= 0;
}

static inline int await_for_output() {
    int timeout = 1000;
    while (!is_output_ready() && timeout > 0) {
        timeout--;
        iowait();
    }

    return timeout <= 0;
}

static inline void clear_fifo() {
    while (is_output_ready()) inb(0x60);
}

uword_t ps2pc_irq(struct handler_args *args) {
    uint8_t status;
    while ((status = inb(0x64)) & 0x1) {
        uint8_t *fifo = status & 0x20 ? ps2pc_auxfifo : ps2pc_kbfifo;
        size_t *index = status & 0x20 ? &ps2pc_auxfifo_i : &ps2pc_kbfifo_i;

        uint8_t byte = inb(0x60);
        //printf("IRQ: %x\n", (uint32_t) byte);
        fifo[(*index)++] = byte;

        if (*index >= meminfo.pagesize) {
            memcpy(fifo, fifo + 1, meminfo.pagesize - 1);
            (*index)--;
            fifo[*index] = 0;
        }
    }

    return 0;
}

static inline void ps2pc_clear_fifo() {
    memset(ps2pc_kbfifo, 0, meminfo.pagesize);
    memset(ps2pc_auxfifo, 0, meminfo.pagesize);

    ps2pc_auxfifo_i = 0;
    ps2pc_kbfifo_i = 0;
}

int ps2pc_command(uint8_t cmd, int response, uint8_t *arg) {
    clear_fifo();
    if (await_for_input()) return -EIO;
    outb(0x64, cmd);

    if (arg) {
        if (await_for_input() || !is_for_controller()) return -EIO;
        outb(0x60, *arg);
    }

    if (response) {
        if (await_for_output()) return -EIO;
        return inb(0x60);
    }

    return 0;
}

int ps2pc_send(uint8_t port, uint8_t byte) {
    if (port > ps2pc_dualport) return -EINVAL;
    if (port == 1 && ps2pc_command(0xd4, 0, NULL)) return -EIO;
    if (await_for_input()) return -EIO;

    outb(0x60, byte);
    return 0;
}

static inline int _ps2pc_command_on_port(uint8_t port, uint8_t cmd, uint8_t *param, size_t fifo_i, uint8_t *fifo, size_t *index) {
    int ret;

    if ((ret = ps2pc_send(port, cmd))) return ret;
    if (param && (ret = ps2pc_send(port, *param))) return ret;

    int timeout = 100;
    while (*index == fifo_i && timeout > 0) {
        timeout--;
        suspend();
    }

    return timeout > 0 ? fifo_i : -EIO;
}

int ps2pc_command_on_port(uint8_t port, uint8_t cmd, uint8_t *param) {
    uint8_t *fifo = port ? ps2pc_auxfifo : ps2pc_kbfifo;
    size_t *index = port ? &ps2pc_auxfifo_i : &ps2pc_kbfifo_i;

    int att = 5, id = 0;
    clear_fifo();
    while (att > 0) {
        size_t fifo_i = port ? ps2pc_auxfifo_i : ps2pc_kbfifo_i;
        id = _ps2pc_command_on_port(port, cmd, param, fifo_i, fifo, index);
        if (id < 0) return id;

        if (fifo[id] == 0xfa)
            break;
        else if (fifo[id] != 0xfe) {
            printf("// PS/2 received unknown byte %x from port %u\n", (uint32_t) fifo[id], (uint32_t) port);
            return -EIO;
        }

        att++;
    }

    if (!att) {
        printf("// PS/2 port %u timeout\n", (uint32_t) port);
        return -EIO;
    }

    return id + 1;
}

int ps2pc_identify(uint8_t port, uint8_t *fifo) {
    int id = ps2pc_command_on_port(port, 0xff, NULL);

    if (id >= 0) {
        uint8_t data = fifo[id];

        if (data == 0xaa || !data) {
            if (port == 0) ps2pc_command_on_port(port, 0xf5, NULL);
            iowait();
            iowait();
            id = ps2pc_command_on_port(port, 0xf2, NULL);
            uint16_t *type = (uint16_t *) (port == 1 ? &ps2pc_auxfifo[id] : &ps2pc_kbfifo[id]);
            iowait();
            iowait();
            if (port == 0) ps2pc_command_on_port(port, 0xf4, NULL);
            if (id < 0) return id;
            iowait();
            iowait();

            switch (*type & 0xff) {
                case 0xab:
                case 0xac: return INPUT_KYBRD;

                case 0x0:
                case 0x3:
                case 0x4: return INPUT_MOUSE;

                default: return INPUT_INAUX;
            }
        }
    }

    return -EIO;
}

static inline int register_node(uint8_t port, uint8_t type, uint8_t *fifo) {
    struct nirux_input_device dev = {
        .drv = &input_ps2pc,
        .fifo = fifo,
        .fifosiz = meminfo.pagesize,
        .type = (type & 0xf) | ((uint16_t) port << 4)
    };

    return input_interface.modctl(MODCTLREGIST, &dev);
}

int ps2pc_initialise() {
    uint8_t status = inb(0x64);
    if (!status || status == 0xff) return 0;

    if (ps2pc_command(0xad, 0, NULL)) return -EIO;
    if (ps2pc_command(0xa7, 0, NULL)) return -EIO;

    while (is_output_ready()) inb(0x60);

    int ccb = ps2pc_command(0x20, 1, NULL);
    if (ccb < 0) return ccb;

    uint8_t ccb8 = ccb & 0xff;
    ccb8 &= ~0x3;
    ccb8 &= ~(1 << 6);
    ccb8 &= ~(1 << 4);
    if (ps2pc_command(0x60, 0, &ccb8)) return -EIO;

    int selftest = ps2pc_command(0xaa, 1, NULL);
    if (selftest < 0) return -EIO;
    if (selftest != 0x55) printf("// PS/2 controller self-test failure: %d\n", selftest);

    if (ps2pc_command(0xa8, 0, NULL)) return -EIO;
    int ccb2 = ps2pc_command(0x20, 1, NULL);
    ccb8 = ccb2;

    if (!(ccb2 & (1 << 5))) {
        ps2pc_dualport = 1;
        ps2pc_command(0xa7, 0, NULL);
        printf("// PS/2 dual channel controller\n");
    } else printf("// PS/2 single channel controller\n");

    int p0 = ps2pc_command(0xab, 1, NULL);
    if (!p0) {
        if (ps2pc_command(0xae, 0, NULL)) return -EIO;
        printf("// PS/2 enabled port 0\n");

        ps2pc_port0 = 1;
        ccb8 |= 0x1;
        ccb8 &= ~(1 << 4);
        ccb8 &= ~(1 << 6);

        ps2pc_kbfifo = kalloc(1);
        if (!ps2pc_kbfifo) return -ENOMEM;

        register_irq(0x21, ps2pc_irq, 0);
        set_irq_state(0x21, 1);
    } else if (p0 < 0) return p0;

    if (ps2pc_dualport) {
        int p1 = ps2pc_command(0xa9, 1, NULL);
        if (!p1) {
            if (ps2pc_command(0xa8, 0, NULL)) return -EIO;
            printf("// PS/2 enabled port 1\n");

            ps2pc_port1 = 1;
            ccb8 |= 0x2;
            ccb8 &= ~(1 << 5);

            ps2pc_auxfifo = kalloc(1);
            if (!ps2pc_auxfifo) {
                kfree(ps2pc_kbfifo);
                return -ENOMEM;
            }

            register_irq(0x2c, ps2pc_irq, 0);
            set_irq_state(0x2c, 1);
        } else if (p1 < 0) return p1;
    }

    if (ps2pc_command(0x60, 0, &ccb8)) return -EIO;

    ps2pc_clear_fifo();

    if (ps2pc_port0) {
        int type = ps2pc_identify(0, ps2pc_kbfifo);
        if (type >= 0 && register_node(0, type, ps2pc_kbfifo)) type = -1;

        if (type < 0) {
            ps2pc_port0 = 0;
            ps2pc_command(0xad, 0, NULL);
        } else {
            printf("// PS/2 device on port 0 acknowledged - type: %x\n", (uint32_t) type);
        }

        ps2pc_clear_fifo();
    }

    if (ps2pc_port1) {
        int type = ps2pc_identify(1, ps2pc_auxfifo);
        if (type >= 0 && register_node(1, type, ps2pc_auxfifo)) type = -1;

        if (type < 0) {
            ps2pc_port1 = 0;
            ps2pc_command(0xa7, 0, NULL);
        } else {
            printf("// PS/2 device on port 1 acknowledged - type: %x\n", (uint32_t) type);
        }

        ps2pc_clear_fifo();
    }

    if (!ps2pc_port0) kfree(ps2pc_kbfifo);
    if (!ps2pc_port1) kfree(ps2pc_auxfifo);

    return 0;
}

ssize_t ps2pc_await(uint16_t addr, void *restrict buf, size_t n) {

}

struct nirux_input_driver input_ps2pc = {
    .name = "ps2pc",
    .initialise = ps2pc_initialise,
    .await = ps2pc_await,
};
