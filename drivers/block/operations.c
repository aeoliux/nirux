#include <block.h>
#include <string.h>
#include <device.h>
#include <errno.h>

uint8_t block_read_buffer[4096];;
ssize_t block_read_bytes(struct nirux_block_device *blk, struct nirux_block_driver *drv, size_t seek, void *restrict buf, size_t n) {
    size_t middle_sectors_start = (seek / blk->sector_size) + ((seek % blk->sector_size) > 0);
    size_t middle_sectors_count = (n / blk->sector_size) - (((n + seek) % blk->sector_size) > 0);
    size_t middle_sectors_limit = middle_sectors_start + middle_sectors_count;

    size_t left_remaining_bytes = (middle_sectors_start * blk->sector_size) - seek;
    size_t rght_remaining_bytes = (seek + n) - (middle_sectors_limit * blk->sector_size);

    ssize_t left_nread = 0;
    if (left_remaining_bytes) {
        left_nread = drv->read(blk, middle_sectors_start - 1, block_read_buffer, 1);
        if (left_nread < 1)
            return -EIO;

        memcpy(buf, block_read_buffer + (seek % blk->sector_size), left_remaining_bytes);
    }

    ssize_t nread = 0;
    if (middle_sectors_count) {
        nread = drv->read(blk, middle_sectors_start, buf + left_remaining_bytes, middle_sectors_count);
        if (nread < (ssize_t) middle_sectors_count) {
            if (nread < 0)
                return left_remaining_bytes ? (ssize_t) left_remaining_bytes : nread;
            else
                return (nread * blk->sector_size) + left_remaining_bytes;
        }
    }

    ssize_t rght_nread = 0;
    if (rght_remaining_bytes) {
        rght_nread = drv->read(blk, middle_sectors_limit, block_read_buffer, 1);
        if (rght_nread < 1) {
            if (nread > 0 || left_remaining_bytes)
                return (nread * blk->sector_size) + left_remaining_bytes;
            else
                return -EIO;
        }

        memcpy(buf + (middle_sectors_count * blk->sector_size), block_read_buffer, rght_remaining_bytes);
    }

    return nread * blk->sector_size + left_remaining_bytes + rght_remaining_bytes;
}

ssize_t block_read_block(struct nirux_block_device *blk, struct nirux_block_driver *drv, size_t seek, void *restrict buf, size_t n) {
    if ((n % blk->sector_size) && n > 4096)
        return -EINVAL;

    void *dst = ((n % blk->sector_size || n < blk->sector_size)) ? block_read_buffer : buf;

    ssize_t result = drv->read(blk, seek / blk->sector_size, dst, n / blk->sector_size);
    if (result < 0)
        return result;
    
    result *= blk->sector_size;

    if (dst == block_read_buffer && result > 0)
        memcpy(buf, dst, result > (ssize_t) n ? (ssize_t) n : result);

    return (size_t) result > n ? (ssize_t) n : result;
}