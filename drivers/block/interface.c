#include <device.h>
#include <errno.h>
#include <modctl.h>
#include <block.h>
#include <driver.h>
#include <string.h>

size_t block_devices_counter = 0;
struct nirux_block_device block_devices[BLOCK_MAX_DEV] = {0};

int block_modctl(uword_t op, void *arg) {
    uint16_t addr;
    
    switch (op) {
        case MODCTLRELOAD:
            return -ENOSUP;
        case BLOCK_MODCTL_REGISTER:
            addr = ((struct nirux_block_device *) arg)->addr;

            if (block_devices_counter + 1 < BLOCK_MAX_DEV && !is_device_node_set(addr / 256, addr % 256)) {
                memcpy(&block_devices[block_devices_counter++], arg, sizeof(struct nirux_block_device));
                return populate_device_node(addr / 256, addr % 256);
            }
            
            return -ENOMEM;
    }

    return -EINVAL;
}

int block_init(void *arg) {
    return claim_device_node(8, &block_interface);
}

struct nirux_interface_driver block_interface = {
    .name = "block",
    .restrict_fd_access = 0,

    .init = block_init,
    .read = NULL,
    .write = NULL,
    .ioctl = NULL,
    .modctl = block_modctl
};

