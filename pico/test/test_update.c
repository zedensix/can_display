#include "check.h"
#include "store/update.h"

// A ring of 8 sectors, like NOR flash: programming only clears bits.
static uint8_t ring[8 * FLASHLOG_SECTOR];

static void ring_read(void *ctx, uint32_t off, uint8_t *out, size_t len) {
    (void)ctx;
    memcpy(out, ring + off, len);
}
static void ring_program(void *ctx, uint32_t off, const uint8_t page[FLASHLOG_PAGE]) {
    (void)ctx;
    for (int i = 0; i < FLASHLOG_PAGE; i++) {
        ring[off + i] &= page[i];
    }
}
static void ring_erase(void *ctx, uint32_t off) {
    (void)ctx;
    memset(ring + off, 0xFF, FLASHLOG_SECTOR);
}

static bool all_erased(uint32_t sector) {
    for (int i = 0; i < FLASHLOG_SECTOR; i++) {
        if (ring[sector * FLASHLOG_SECTOR + i] != 0xFF) {
            return false;
        }
    }
    return true;
}

void test_update(void) {
    flash_dev_t dev = {NULL, sizeof ring, ring_read, ring_program, ring_erase};
    CHECK(update_crc32(0, (const uint8_t *)"123456789", 9) == 0xCBF43926, "crc32 check value");

    // The log writes in sector 5: the header goes in 6, the image in 7, 0
    // and 1 (round the end), sector 5 untouched.
    memset(ring, 0x00, sizeof ring);
    static uint8_t image[2 * FLASHLOG_SECTOR + 1000];
    for (size_t i = 0; i < sizeof image; i++) {
        image[i] = (uint8_t)(i * 7 + 3);
    }
    uint32_t crc = update_crc32(0, image, sizeof image);
    update_t u;
    CHECK(!update_begin(&u, dev, 5 * FLASHLOG_SECTOR + 100, 7 * FLASHLOG_SECTOR, crc), "too big for the ring");
    CHECK(!update_begin(&u, dev, 0, 0, crc), "empty image");
    CHECK(update_begin(&u, dev, 5 * FLASHLOG_SECTOR + 100, sizeof image, crc), "fits");
    CHECK(u.header == 6 * FLASHLOG_SECTOR, "header after the log's sector: %u", (unsigned)u.header);
    CHECK(update_image_offset(&u, 0) == 7 * FLASHLOG_SECTOR && update_image_offset(&u, 1) == 0 &&
              update_image_offset(&u, 2) == FLASHLOG_SECTOR,
          "image wraps round the ring");

    // In chunks of 239 (the phone's), one out of order refused.
    CHECK(!update_write(&u, 239, image + 239, 239), "a write past received is refused");
    for (uint32_t at = 0; at < sizeof image; at += 239) {
        uint32_t n = sizeof image - at < 239 ? sizeof image - at : 239;
        CHECK(update_write(&u, at, image + at, n), "write at %u", (unsigned)at);
    }
    CHECK(!update_write(&u, sizeof image, image, 1), "nothing past the end");
    CHECK(update_check(&u), "image staged with its CRC");
    CHECK(ring[5 * FLASHLOG_SECTOR] == 0x00, "the log's own sector untouched");
    CHECK(memcmp(ring + FLASHLOG_SECTOR, image + 2 * FLASHLOG_SECTOR, 1000) == 0, "last sector holds the tail");
    CHECK(ring[FLASHLOG_SECTOR + 1000] == 0xFF, "last page padded");

    update_t found;
    CHECK(update_find(&found, dev) && found.header == u.header && found.size == u.size && found.crc == crc,
          "the next boot finds the header");
    update_erase(&found, true);
    CHECK(all_erased(6) && all_erased(7) && all_erased(0) && all_erased(1), "image and header erased");
    CHECK(!all_erased(2) && !all_erased(5), "the rest of the ring kept");
    CHECK(!update_find(&found, dev), "no header left");

    // A wrong CRC fails the check; cancelled early, only what was written goes.
    memset(ring, 0x00, sizeof ring);
    CHECK(update_begin(&u, dev, 0, sizeof image, crc ^ 1), "begin");
    for (uint32_t at = 0; at < sizeof image; at += 1000) {
        uint32_t n = sizeof image - at < 1000 ? sizeof image - at : 1000;
        update_write(&u, at, image + at, n);
    }
    CHECK(!update_check(&u), "CRC mismatch caught");
    CHECK(update_begin(&u, dev, 0, sizeof image, crc), "begin again");
    update_write(&u, 0, image, 100);
    CHECK(!update_check(&u), "incomplete image fails");
    update_erase(&u, false);
    CHECK(all_erased(1) && all_erased(2) && !all_erased(3), "only the header and the sector written erased");
}
