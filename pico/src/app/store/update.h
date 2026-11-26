#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "flashlog.h"

// A new firmware on its way from the phone, staged in the data log's
// ring: a header sector, then the image, in the sectors after the one the
// log writes in (its oldest records, already pulled by the phone),
// wrapping round the ring's end. Once it's all there and its CRC matches,
// the firmware copies it over itself and restarts; the next boot finds
// the header and erases what the image took, so the data log never reads
// it as records.
//
// The header, little-endian: u32 UPDATE_MAGIC, u32 the image's size, u32
// its CRC-32, the rest of the sector erased.

#define UPDATE_MAGIC 0x31445055u

typedef struct {
    flash_dev_t dev;
    // header is its sector's offset in dev; received counts the image's
    // bytes taken, page holds those past the last whole page.
    uint32_t header, size, crc, received;
    uint8_t page[FLASHLOG_PAGE];
} update_t;

// update_crc32 carries crc (0 to start) over data: zlib's CRC-32, the
// phone's java.util.zip.CRC32.
uint32_t update_crc32(uint32_t crc, const uint8_t *data, size_t len);

// update_begin stages size bytes whose CRC-32 is crc in dev, the data
// log's region, after the sector holding log_pos, where the log writes
// next: it writes the header. False, flash untouched, if size is 0 or
// the image, its header and the log's own sector don't fit in the ring.
bool update_begin(update_t *u, flash_dev_t dev, uint32_t log_pos, uint32_t size, uint32_t crc);

// update_write takes len bytes of the image at offset, which must be
// received (anything else is refused: false), erasing each sector as it's
// entered and programming each page as it fills, the last one padded.
bool update_write(update_t *u, uint32_t offset, const uint8_t *data, size_t len);

// update_check is whether the whole image is in flash with its CRC.
bool update_check(const update_t *u);

// update_image_offset is where image sector i sits in dev.
uint32_t update_image_offset(const update_t *u, uint32_t i);

// update_erase erases the image's sectors, every one or only those
// received reached, then the header's: a cut short erase leaves the
// header for the next boot to finish.
void update_erase(update_t *u, bool every_sector);

// update_find looks for a header in dev; true and u filled if there is
// one (received 0).
bool update_find(update_t *u, flash_dev_t dev);
