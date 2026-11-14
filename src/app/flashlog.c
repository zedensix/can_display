#include "flashlog.h"

#define ERASED 0xFF

static uint8_t read_byte(const flashlog_t *log, uint32_t offset) {
    uint8_t b;
    log->dev.read(log->dev.ctx, offset, &b, 1);
    return b;
}

// find_end returns the first erased byte that follows a written one, and
// whether the rest of its sector is blank, as appends need it to be.
static bool find_end(const flashlog_t *log, uint32_t *end) {
    uint32_t size = log->dev.size;
    uint8_t prev = read_byte(log, size - 1);
    bool found = false, all_erased = true;
    uint8_t page[FLASHLOG_PAGE];
    for (uint32_t off = 0; off < size; off += FLASHLOG_PAGE) {
        log->dev.read(log->dev.ctx, off, page, FLASHLOG_PAGE);
        for (uint32_t i = 0; i < FLASHLOG_PAGE; i++) {
            if (page[i] != ERASED) {
                all_erased = false;
            } else if (prev != ERASED && !found) {
                *end = off + i;
                found = true;
            }
            prev = page[i];
        }
    }
    if (all_erased) {
        *end = 0;
        return true;
    }
    if (!found) {
        return false;
    }
    uint32_t sector_end = (*end / FLASHLOG_SECTOR + 1) * FLASHLOG_SECTOR;
    for (uint32_t off = *end; off < sector_end; off++) {
        if (read_byte(log, off) != ERASED) {
            return false;
        }
    }
    return true;
}

bool flashlog_open(flashlog_t *log, flash_dev_t dev) {
    if (dev.size < 2 * FLASHLOG_SECTOR || dev.size % FLASHLOG_SECTOR != 0) {
        return false;
    }
    log->dev = dev;
    if (!find_end(log, &log->pos)) {
        for (uint32_t off = 0; off < dev.size; off += FLASHLOG_SECTOR) {
            dev.erase_sector(dev.ctx, off);
        }
        log->pos = 0;
    }
    return true;
}

static void append_byte_run(flashlog_t *log, const uint8_t *data, size_t len) {
    uint8_t page[FLASHLOG_PAGE];
    while (len > 0) {
        // The flash only programs whole pages: read the page back, add to
        // it, program it whole (rewriting a byte with itself is a no-op).
        uint32_t page_start = log->pos / FLASHLOG_PAGE * FLASHLOG_PAGE;
        log->dev.read(log->dev.ctx, page_start, page, FLASHLOG_PAGE);
        size_t n = 0;
        for (uint32_t i = log->pos - page_start; i < FLASHLOG_PAGE && n < len; i++, n++) {
            page[i] = data[n] == ERASED ? '?' : data[n];
        }
        log->dev.program_page(log->dev.ctx, page_start, page);
        data += n;
        len -= n;
        log->pos = (log->pos + (uint32_t)n) % log->dev.size;
        // Erasing the next sector as soon as it's reached keeps a blank
        // gap after the newest line, which flashlog_open looks for.
        if (log->pos % FLASHLOG_SECTOR == 0) {
            log->dev.erase_sector(log->dev.ctx, log->pos);
        }
    }
}

void flashlog_append(flashlog_t *log, const char *line) {
    size_t len = 0;
    while (line[len]) {
        len++;
    }
    append_byte_run(log, (const uint8_t *)line, len);
    append_byte_run(log, (const uint8_t *)"\n", 1);
}

void flashlog_dump(const flashlog_t *log, void (*write)(void *ctx, const uint8_t *data, size_t len), void *ctx) {
    // The rest of the newest sector is blank, so the oldest line starts
    // at the next sector.
    uint32_t start = (log->pos / FLASHLOG_SECTOR + 1) * FLASHLOG_SECTOR;
    uint8_t page[FLASHLOG_PAGE];
    for (uint32_t i = 0; i < log->dev.size; i += FLASHLOG_PAGE) {
        log->dev.read(log->dev.ctx, (start + i) % log->dev.size, page, FLASHLOG_PAGE);
        size_t n = 0;
        for (size_t j = 0; j < FLASHLOG_PAGE; j++) {
            if (page[j] != ERASED) {
                page[n++] = page[j];
            }
        }
        if (n > 0) {
            write(ctx, page, n);
        }
    }
}
