#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A short text log in a ring of flash sectors, so what happened in the
// car can be read back at the desk. Text never contains 0xFF, the value
// of erased flash: the first erased byte after a written one is where the
// previous run stopped.

#define FLASHLOG_SECTOR 4096
#define FLASHLOG_PAGE 256

// flash_dev_t is the flash region holding the log, offsets counted from
// its start. Like NOR flash, program can only clear bits, and erase sets
// a whole sector back to 0xFF.
typedef struct {
    void *ctx;
    uint32_t size;
    void (*read)(void *ctx, uint32_t offset, uint8_t *out, size_t len);
    void (*program_page)(void *ctx, uint32_t offset, const uint8_t page[FLASHLOG_PAGE]);
    void (*erase_sector)(void *ctx, uint32_t offset);
} flash_dev_t;

typedef struct {
    flash_dev_t dev;
    uint32_t pos;
} flashlog_t;

// flashlog_open picks the log up where the previous run left it. A region
// that doesn't hold a log (another firmware's leftovers) is erased. size
// must be at least two whole sectors.
bool flashlog_open(flashlog_t *log, flash_dev_t dev);

// flashlog_append appends line and a newline, overwriting the oldest
// sector once the region is full.
void flashlog_append(flashlog_t *log, const char *line);

// flashlog_dump writes the whole log, oldest line first. Once the log has
// wrapped, the first line can be cut: its start went with the sector
// overwritten last.
void flashlog_dump(const flashlog_t *log, void (*write)(void *ctx, const uint8_t *data, size_t len), void *ctx);
