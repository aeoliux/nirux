#pragma once

#include <driver.h>

#define NINPUTDRV_FLAG_MOUS 0x0001
#define NINPUTDRV_FLAG_KEYB 0x0002
#define NINPUTDRV_FLAG_CUST 0x0004
#define NINPUTDRV_FLAG_POLL 0x1000

#define NINPUTDRV_FFSI 0x0100

struct nirux_input_driver {
    // Device driver name
    uint8_t name[DRVNAMLEN + 1];
    // Device flags
    uint16_t flag;
    
    // If pollable flag is not set
    uint16_t fifo[NINPUTDRV_FFSI];

    // If pollable flag is set
    uint16_t (*poll)(void *arg);
} __attribute__((__packed__));
