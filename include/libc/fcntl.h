#pragma once

enum file_access_modes {
    O_RDONLY        = 0x1,
    O_WRONLY        = 0x2,
    O_RDWR          = O_RDONLY | O_WRONLY,
    O_APPEND        = O_WRONLY | 0x4,
    O_CREAT         = 0x8,
    O_DIRECTORY     = 0x10,
};