#pragma once

#include <stdint.h>
#include <irq.h>
#include <block.h>

enum x86floppy_commands {
    X86FLOPPY_CMD_READ_TRACK =                 2,
    X86FLOPPY_CMD_SPECIFY =                    3,
    X86FLOPPY_CMD_SENSE_DRIVE_STATUS =         4,
    X86FLOPPY_CMD_WRITE_DATA =                 5,
    X86FLOPPY_CMD_READ_DATA =                  6,
    X86FLOPPY_CMD_RECALIBRATE =                7,
    X86FLOPPY_CMD_SENSE_INTERRUPT =            8,
    X86FLOPPY_CMD_WRITE_DELETED_DATA =         9,
    X86FLOPPY_CMD_READ_ID =                    10,
    X86FLOPPY_CMD_READ_DELETED_DATA =          12,
    X86FLOPPY_CMD_FORMAT_TRACK =               13,
    X86FLOPPY_CMD_DUMPREG =                    14,
    X86FLOPPY_CMD_SEEK =                       15,
    X86FLOPPY_CMD_VERSION =                    16,
    X86FLOPPY_CMD_SCAN_EQUAL =                 17,
    X86FLOPPY_CMD_PERPENDICULAR_MODE =         18,
    X86FLOPPY_CMD_CONFIGURE =                  19,
    X86FLOPPY_CMD_LOCK =                       20,
    X86FLOPPY_CMD_VERIFY =                     22,
    X86FLOPPY_CMD_SCAN_LOW_OR_EQUAL =          25,
    X86FLOPPY_CMD_SCAN_HIGH_OR_EQUAL =         29,

    X86FLOPPY_CMD_EOA =                        0x100,
};

enum x86floppy_ioregs {
    X86FLOPPY_REG_STATUS_REGISTER_A                = 0x3F0, // read-only
    X86FLOPPY_REG_STATUS_REGISTER_B                = 0x3F1, // read-only
    X86FLOPPY_REG_DIGITAL_OUTPUT_REGISTER          = 0x3F2,
    X86FLOPPY_REG_TAPE_DRIVE_REGISTER              = 0x3F3,
    X86FLOPPY_REG_MAIN_STATUS_REGISTER             = 0x3F4, // read-only
    X86FLOPPY_REG_DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
    X86FLOPPY_REG_DATA_FIFO                        = 0x3F5,
    X86FLOPPY_REG_DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
    X86FLOPPY_REG_CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
};

extern uint8_t x86floppy_irqed;
extern uint8_t *x86floppy_dma;
uword_t x86floppy_irq6(struct handler_args *args);

uword_t x86floppy_flush_buffer();
int x86floppy_init(uint32_t *drives, size_t n);
int x86floppy_reset();
void x86floppy_set_motor(uint32_t drive, uint8_t state);
int x86floppy_select_drive(uint32_t drive);
int x86floppy_perform_command(uint8_t command, ...);
int x86floppy_perform_dma(uint8_t is_write, uint8_t spt, uint8_t cylinder, uint8_t head, uint8_t sec);
int x86floppy_perform(uint32_t drive, uint8_t command, uint32_t lba, void *restrict buf, size_t n);
