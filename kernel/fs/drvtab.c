#include <driver.h>
#include <device.h>
#include <fs/fat.h>
#include <fs.h>
#include <string.h>

struct fs_driver_entry fs_drvs[] = {
    [0] = {
        "fat",
        &fs_fat
    },
    [1] = {
        "devfs",
        &fs_devfs
    },
    [2] = {
        "",
        NULL
    }
};

struct fs_driver_entry *fs_drivers = fs_drvs;

struct fs_driver *fs_get_driver(const char *fsname) {
    for (size_t i = 0; fs_drvs[i].drv; i++)
        if (!strncmp(fsname, fs_drvs[i].name, DRVNAMLEN + 1))
            return fs_drvs[i].drv;

    return NULL;
}