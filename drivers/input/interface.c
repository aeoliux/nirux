#include <device.h>
#include <driver.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <input.h>
#include <modctl.h>

extern struct nirux_interface_driver input_interface;

struct nirux_input_device input_kybrd_fifo[16];
struct nirux_input_device input_mouse_fifo[16];
struct nirux_input_device input_inaux_fifo[16];
static size_t kybrd_n = 0, mouse_n = 0, inaux_n = 0;

ssize_t input_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {

    return -ENOSUP;
}

ssize_t input_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {

    return -ENOSUP;
}

int input_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {

    return -ENOSUP;
}

int input_modctl(uword_t op, void *arg) {
    struct nirux_input_device *dev;
    switch (op) {
        case MODCTLREGIST:
            dev = (struct nirux_input_device *) arg;
            if (!dev->type) return -EINVAL;

            uint8_t major;
            size_t *index;
            struct nirux_input_device *arr;
            switch (dev->type & 0xf) {
                case INPUT_KYBRD:
                    major = 1;
                    index = &kybrd_n;
                    arr = input_kybrd_fifo;
                    break;
                case INPUT_MOUSE:
                    major = 2;
                    index = &mouse_n;
                    arr = input_mouse_fifo;
                    break;
                case INPUT_INAUX:
                    major = 3;
                    index = &inaux_n;
                    arr = input_inaux_fifo;
                    break;
                default: return -EINVAL;
            }

            if (*index >= 16) return -ENOMEM;

            memcpy(&arr[*index], dev, sizeof(struct nirux_input_device));
            populate_device_node(major, (*index)++);

            return 0;
        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                switch (major) {
                    case 1: if (minor >= kybrd_n) return -ENXIO; memcpy(arg, "kbd", 4); break;
                    case 2: if (minor >= mouse_n) return -ENXIO; memcpy(arg, "mous", 5); break;
                    case 3: if (minor >= inaux_n) return -ENXIO; memcpy(arg, "inaux", 6); break;
                    default: return -EINVAL;
                }

                size_t length = strlen(arg);
                if (length < DRVNAMLEN - 2) {
                    i32toa(arg + strlen(arg), (uint32_t) minor, 10, 1);
                }

                return 0;
            }
    }

    return -EINVAL;
}

int input_init(void *arg) {
    claim_device_node(1, &input_interface);
    claim_device_node(2, &input_interface);
    return 0;
}

struct nirux_interface_driver input_interface = {
    .name = "input",
    .init = input_init,
    .read = input_read,
    .write = input_write,
    .ioctl = input_ioctl,
    .modctl = input_modctl,
};
