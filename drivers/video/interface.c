#include <modctl.h>
#include <errno.h>
#include <driver.h>
#include <fbdev.h>
#include <stdint.h>
#include <string.h>
#include <device.h>
#include <stdio.h>

extern struct nirux_framebuffer_driver vga_framebuffer_driver;
struct nirux_framebuffer_driver *framebuffer_drivers[] = { &vga_framebuffer_driver };

struct framebuffer_minor_node {
    struct nirux_framebuffer_driver *driver;
    uint8_t output;
} __attribute__((__packed__)) framebuffer_devices[NFBDEV_MAX_OUTPUTS] = {0};
uint8_t framebuffer_devices_counter = 0;

ssize_t video_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    return -ENOSUP;
}

ssize_t video_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    struct framebuffer_minor_node *fbnode = &framebuffer_devices[minor];
    if (framebuffer_devices_counter <= minor || !fbnode->driver || !fbnode->driver->outputs_count)
        return -ENXIO;

    struct nirux_framebuffer_output *display = &fbnode->driver->outputs[fbnode->output];
    if (!display->current_mode)
        return -ENXIO;

    return fbnode->driver->write(fbnode->output, seek, buf, n);
}

int video_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    struct framebuffer_minor_node *fbnode = &framebuffer_devices[minor];
    if (framebuffer_devices_counter <= minor || !fbnode->driver || !fbnode->driver->outputs_count)
        return -ENXIO;

    struct nirux_framebuffer_output *display = &fbnode->driver->outputs[fbnode->output];
    if (!display->current_mode)
        return -ENXIO;

    switch (op) {
        case IOFBDEVMOVCUR:
            if (display->current_mode == NFBDEV_CURMODE_TEXTBF) {
                uint16_t *cursor_pos = (uint16_t *) arg;

                // word x : word y
                uint16_t x = cursor_pos[0];
                uint16_t y = cursor_pos[1];

                return fbnode->driver->cursor_move(fbnode->output, x, y);
            }
            break;
    
        case IOFBDEVTXTSET:
            if (display->current_mode == NFBDEV_CURMODE_TEXTBF) {
                size_t *seek_and_count = (size_t *) arg;

                // dword seek : dword count : word entry
                size_t seek = seek_and_count[0];
                size_t count = seek_and_count[1];

                uint16_t entry = *((uint16_t *) (arg + (2 * sizeof(size_t))));
                return count > 1 
                    ? fbnode->driver->set16(fbnode->output, seek, entry, count)
                    : fbnode->driver->set16single(fbnode->output, seek, entry);
            }
            break;
        
        case IOFBDEVTXSCRL:
            if (display->current_mode == NFBDEV_CURMODE_TEXTBF)
                return fbnode->driver->scroll(fbnode->output, *((size_t *) arg));
            break;
        
        case IOFBDEVGETCUR:
            if (display->current_mode == NFBDEV_CURMODE_TEXTBF) {
                uint16_t x = fbnode->driver->outputs[fbnode->output].cursor_x;
                uint16_t y = fbnode->driver->outputs[fbnode->output].cursor_y;

                *((uint32_t *) arg) = (((uint32_t) y << 16) & 0xffff0000) | x;
                return 0;
            }
            break;

        case IOFBDEVGETRES:
            *((uint32_t *) arg) = 
                (((uint32_t) fbnode->driver->outputs[fbnode->output].height << 16) & 0xffff0000) |
                fbnode->driver->outputs[fbnode->output].width;
            return 0;

        case IOFBDEVBFADDR:
            if (display->current_mode == NFBDEV_CURMODE_TEXTBF) {

                *((uint16_t **) arg) = (uint16_t *) fbnode->driver->outputs[fbnode->output].buffer;
                return 0;
            }
            break;

        case IOFBDEVMODSET:
            if ((fbnode->driver->flag & NFBDEV_FLAG_KMODESET) && fbnode->driver->modeset)
                return fbnode->driver->modeset(fbnode->output, (uint32_t *) arg);
            break;
    }

    return -ENOSUP;
}

int video_modctl(uword_t op, void *arg) {
    switch (op) {
        case MODCTLRELOAD:
            for (int i = 0; i < framebuffer_devices_counter; i++)
                if (!framebuffer_devices[i].driver->outputs[framebuffer_devices[i].output].current_mode) {
                    memset(&framebuffer_devices[i], 0, sizeof(struct framebuffer_minor_node));

                    depopulate_device_node(NIRUX_FBDEV_NODE, i);
                }

            framebuffer_devices_counter = 0;
            
            for (int i = 0; i < 1; i++) {
                struct nirux_framebuffer_driver *fbdev = framebuffer_drivers[i];

                for (uint8_t j = 0; j < fbdev->outputs_count && j < NFBDEV_MAX_OUTPUTS; j++) {
                    if (fbdev->outputs[j].current_mode) {
                        framebuffer_devices[framebuffer_devices_counter].driver = fbdev;
                        framebuffer_devices[framebuffer_devices_counter].output = j;

                        // register node
                        populate_device_node(NIRUX_FBDEV_NODE, framebuffer_devices_counter++);
                    }
                }
            }

            return 0;

        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                struct framebuffer_minor_node *fbnode = &framebuffer_devices[minor];
                if (framebuffer_devices_counter <= minor || !fbnode->driver || !fbnode->driver->outputs_count)
                    return -ENXIO;

                memcpy(arg, fbnode->driver->name, DRVNAMLEN + 1);
                size_t length = strlen(arg);
                if (length < DRVNAMLEN - 2) {
                    i32toa(arg + strlen(arg), (uint32_t) minor, 10, 1);
                }

                return 0;
            }
    }

                    
    return -ENOSUP;
}

int video_init(void *arg) {
    return claim_device_node(29, &video_interface);
}

struct nirux_interface_driver video_interface = {
    .name = "video",
    .restrict_fd_access = 0,
    .init = video_init,
    .read = video_read,
    .write = video_write,
    .ioctl = video_ioctl,
    .modctl = video_modctl
};