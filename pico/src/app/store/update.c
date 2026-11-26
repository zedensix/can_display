#include "update.h"

#include <string.h>

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_u32_le(uint8_t *p, uint32_t v) {
    for (int i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

uint32_t update_crc32(uint32_t crc, const uint8_t *data, size_t len) {
    // Four bits at a time: a 64-byte table, fast enough for a megabyte.
    static const uint32_t table[16] = {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
    };
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc = table[(crc ^ data[i]) & 0x0F] ^ (crc >> 4);
        crc = table[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}

static uint32_t image_sectors(uint32_t size) {
    return (size + FLASHLOG_SECTOR - 1) / FLASHLOG_SECTOR;
}

uint32_t update_image_offset(const update_t *u, uint32_t i) {
    return (u->header + (1 + i) * FLASHLOG_SECTOR) % u->dev.size;
}

bool update_begin(update_t *u, flash_dev_t dev, uint32_t log_pos, uint32_t size, uint32_t crc) {
    uint32_t sectors = dev.size / FLASHLOG_SECTOR;
    if (size == 0 || size > dev.size || 1 + image_sectors(size) > sectors - 1) {
        return false;
    }
    *u = (update_t){.dev = dev, .size = size, .crc = crc};
    u->header = (log_pos / FLASHLOG_SECTOR + 1) % sectors * FLASHLOG_SECTOR;
    memset(u->page, 0xFF, sizeof u->page);
    write_u32_le(u->page, UPDATE_MAGIC);
    write_u32_le(u->page + 4, size);
    write_u32_le(u->page + 8, crc);
    dev.erase_sector(dev.ctx, u->header);
    dev.program_page(dev.ctx, u->header, u->page);
    memset(u->page, 0xFF, sizeof u->page);
    return true;
}

static void program_page(update_t *u, uint32_t image_offset) {
    uint32_t at = update_image_offset(u, image_offset / FLASHLOG_SECTOR) + image_offset % FLASHLOG_SECTOR;
    u->dev.program_page(u->dev.ctx, at, u->page);
    memset(u->page, 0xFF, sizeof u->page);
}

bool update_write(update_t *u, uint32_t offset, const uint8_t *data, size_t len) {
    if (offset != u->received || len > u->size - u->received) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (u->received % FLASHLOG_SECTOR == 0) {
            u->dev.erase_sector(u->dev.ctx, update_image_offset(u, u->received / FLASHLOG_SECTOR));
        }
        u->page[u->received % FLASHLOG_PAGE] = data[i];
        u->received++;
        if (u->received % FLASHLOG_PAGE == 0) {
            program_page(u, u->received - FLASHLOG_PAGE);
        }
    }
    if (u->received == u->size && u->received % FLASHLOG_PAGE != 0) {
        program_page(u, u->received - u->received % FLASHLOG_PAGE);
    }
    return true;
}

bool update_check(const update_t *u) {
    if (u->received != u->size) {
        return false;
    }
    uint32_t crc = 0;
    uint8_t chunk[FLASHLOG_PAGE];
    for (uint32_t at = 0; at < u->size; at += sizeof chunk) {
        uint32_t n = u->size - at < sizeof chunk ? u->size - at : sizeof chunk;
        u->dev.read(u->dev.ctx, update_image_offset(u, at / FLASHLOG_SECTOR) + at % FLASHLOG_SECTOR, chunk, n);
        crc = update_crc32(crc, chunk, n);
    }
    return crc == u->crc;
}

void update_erase(update_t *u, bool every_sector) {
    uint32_t sectors = image_sectors(every_sector ? u->size : u->received);
    for (uint32_t i = 0; i < sectors; i++) {
        u->dev.erase_sector(u->dev.ctx, update_image_offset(u, i));
    }
    u->dev.erase_sector(u->dev.ctx, u->header);
}

bool update_find(update_t *u, flash_dev_t dev) {
    for (uint32_t at = 0; at + FLASHLOG_SECTOR <= dev.size; at += FLASHLOG_SECTOR) {
        uint8_t head[12];
        dev.read(dev.ctx, at, head, sizeof head);
        uint32_t size = read_u32_le(head + 4);
        if (read_u32_le(head) == UPDATE_MAGIC && size > 0 && 1 + image_sectors(size) < dev.size / FLASHLOG_SECTOR) {
            *u = (update_t){.dev = dev, .header = at, .size = size, .crc = read_u32_le(head + 8)};
            memset(u->page, 0xFF, sizeof u->page);
            return true;
        }
    }
    return false;
}
