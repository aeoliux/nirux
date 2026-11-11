#include <driver.h>
#include <device.h>
#include <string.h>

struct nirux_device_node device_nodes[256] = {0};

struct nirux_device_node *get_device_node(uint8_t major) {
    return device_nodes[major].iface ? &device_nodes[major] : NULL;
}

int claim_device_node(uint8_t major, struct nirux_interface_driver *iface) {
    if (device_nodes[major].iface)
        return -1;

    device_nodes[major].iface = iface;
    memset(device_nodes[major].minor, 0, sizeof(device_nodes[major].minor));

    return 0;
}

int unclaim_device_node(uint8_t major) {
    device_nodes[major].iface = NULL;
    memset(device_nodes[major].minor, 0, sizeof(device_nodes[major].minor));

    return 0;
}

static inline int flip_minor_node_state(uint8_t major, uint8_t minor, uint8_t state) {
    if (!device_nodes[major].iface)
        return -1;

    uint8_t caddr = minor / 8;
    uint8_t shift = minor % 8;
    uint8_t *cell = &device_nodes[major].minor[caddr];

    if (state)
        *cell |= (1 << shift);
    else
        *cell &= ~(1 << shift);

    return state > 0;
}

int populate_device_node(uint8_t major, uint8_t minor) {
    return flip_minor_node_state(major, minor, 1);
}

int depopulate_device_node(uint8_t major, uint8_t minor) {
    return flip_minor_node_state(major, minor, 0);
}

int is_device_node_set(uint8_t major, uint8_t minor) {
    if (!device_nodes[major].iface)
        return -1;

    uint8_t caddr = minor / 8;
    uint8_t shift = minor % 8;
    uint8_t cell = device_nodes[major].minor[caddr];

    return cell & (1 << shift);
}