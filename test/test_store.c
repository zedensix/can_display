#include <stdlib.h>

#include "check.h"
#include "store/datalog.h"
#include "store/flashlog.h"
#include "store/settings.h"

// fake_flash behaves like NOR flash: programming can only clear bits.
typedef struct {
    uint8_t data[3 * FLASHLOG_SECTOR];
} fake_flash_t;

static void flash_read(void *ctx, uint32_t off, uint8_t *out, size_t len) {
    memcpy(out, ((fake_flash_t *)ctx)->data + off, len);
}
static void flash_program(void *ctx, uint32_t off, const uint8_t page[FLASHLOG_PAGE]) {
    for (int i = 0; i < FLASHLOG_PAGE; i++) {
        ((fake_flash_t *)ctx)->data[off + i] &= page[i];
    }
}
static void flash_erase(void *ctx, uint32_t off) {
    memset(((fake_flash_t *)ctx)->data + off, 0xFF, FLASHLOG_SECTOR);
}

static char dumped[4 * FLASHLOG_SECTOR];
static size_t dumped_len;
static void collect_log_dump(void *ctx, const uint8_t *data, size_t len) {
    (void)ctx;
    memcpy(dumped + dumped_len, data, len);
    dumped_len += len;
    dumped[dumped_len] = 0;
}
static const char *dump_flashlog(flashlog_t *log) {
    dumped_len = 0;
    dumped[0] = 0;
    flashlog_dump(log, collect_log_dump, NULL);
    return dumped;
}

void test_flashlog(void) {
    static fake_flash_t flash;
    memset(flash.data, 0xFF, sizeof flash.data);
    flash_dev_t dev = {&flash, sizeof flash.data, flash_read, flash_program, flash_erase};
    flashlog_t log;
    CHECK(flashlog_open(&log, dev), "open failed");
    flashlog_append(&log, "boot");
    flashlog_append(&log, "ble: connected");
    flashlog_open(&log, dev);
    flashlog_append(&log, "boot again");
    CHECK(strcmp(dump_flashlog(&log), "boot\nble: connected\nboot again\n") == 0, "after a reopen the log is %s",
          dumped);

    for (int i = 0; i < 3000; i++) {
        char line[16];
        snprintf(line, sizeof line, "line %d", i);
        flashlog_append(&log, line);
        if (i == 1500) {
            flashlog_open(&log, dev);
        }
    }
    dump_flashlog(&log);
    // The first line can be cut: its start went with the dropped sector.
    char *first = strchr(dumped, '\n') + 1;
    int previous = -1, count = 0;
    for (char *line = strtok(first, "\n"); line; line = strtok(NULL, "\n"), count++) {
        int n = atoi(line + 5);
        CHECK(previous < 0 || n == previous + 1, "line %d follows line %d", n, previous);
        previous = n;
    }
    CHECK(previous == 2999 && count < 3000, "after wrapping: last line %d, %d lines kept", previous, count);

    for (size_t i = 0; i < sizeof flash.data; i++) {
        flash.data[i] = (uint8_t)(i % 251);
    }
    flashlog_open(&log, dev);
    flashlog_append(&log, "boot");
    CHECK(strcmp(dump_flashlog(&log), "boot\n") == 0, "foreign data wasn't erased: %s", dumped);
}

void test_settings(void) {
    settings_t cfg = {.source = SOURCE_CAN, .brightness = 40, .auto_connect = true, .flip = true, .shift_rpm = 7100};
    uint8_t buf[SETTINGS_SIZE];
    settings_encode(cfg, buf);
    CHECK(settings_equal(settings_decode(buf), cfg), "settings didn't round-trip");
    cfg.source = SOURCE_BLE;
    settings_encode(cfg, buf);
    CHECK(settings_decode(buf).source == SOURCE_BLE, "the BLE source didn't round-trip");
    buf[4] = 1; // saved by a firmware without the shift light
    settings_t old = settings_decode(buf);
    CHECK(old.brightness == 40 && old.flip && old.shift_rpm == 0, "version 1 settings lost (shift %d)", old.shift_rpm);
    cfg.dongle = (dongle_t){true, {0x33, 0xC0, 0x9A, 0x86, 0x42, 0x41}, 0, "IOS-Vlink"};
    settings_encode(cfg, buf);
    CHECK(settings_equal(settings_decode(buf), cfg), "the dongle didn't round-trip");
    buf[4] = 4; // saved before the dongle's name was
    CHECK(settings_decode(buf).dongle.set && settings_decode(buf).dongle.name[0] == 0, "version 4 dongle misread");
    buf[4] = 3; // saved before the dongle was
    CHECK(!settings_decode(buf).dongle.set && settings_decode(buf).brightness == 40, "version 3 settings misread");
    memset(buf, 0xFF, sizeof buf);
    CHECK(settings_equal(settings_decode(buf), SETTINGS_DEFAULT), "erased flash didn't give the defaults");
}

void test_datalog(void) {
    static fake_flash_t f;
    memset(f.data, 0xFF, sizeof f.data);
    flash_dev_t dev = {&f, sizeof f.data, flash_read, flash_program, flash_erase};
    datalog_t log;
    state_t s = {0};
    datalog_open(&log, dev);
    CHECK(log.pos == 0 && log.session == 0, "empty flash: pos %u session %u", (unsigned)log.pos, log.session);
    state_set_reading(&s, SIG_RPM, 3000, 0);
    state_set_reading(&s, SIG_AFR, 14.7f, 0);
    for (uint32_t t = 0; t < 11 * DATALOG_SAMPLE_MS; t += DATALOG_SAMPLE_MS) {
        s.readings[SIG_RPM].updated_ms = s.readings[SIG_AFR].updated_ms = t;
        datalog_sample(&log, &s, t);
    }
    datalog_flush(&log);
    // 11 records: 8 in the first page, 3 flushed in the second.
    CHECK(f.data[8] == (3000 & 0xFF) && f.data[9] == 3000 >> 8, "the first sample's RPM isn't in flash");
    CHECK(f.data[10 * 32 + 2] == DATALOG_SAMPLE && f.data[10 * 32 + 4] == (10 * DATALOG_SAMPLE_MS & 0xFF),
          "the 11th sample isn't in flash");
    CHECK(f.data[8 + 2 * 1] == 0x00 && f.data[8 + 2 * 1 + 1] == 0x80, "a coolant never read isn't -32768");

    // A new run carries on after it, in a new session.
    datalog_open(&log, dev);
    CHECK(log.pos == 11 * 32 && log.session == 1, "reopened: pos %u session %u, want 352 and 1", (unsigned)log.pos,
          log.session);
    datalog_sample(&log, &s, 0);
    datalog_flush(&log);
    CHECK(f.data[11 * 32] == 1 && f.data[10 * 32] == 0, "session 1 didn't follow session 0");

    // Read back in ring order, the oldest first.
    uint8_t first[DATALOG_RECORD];
    datalog_read(&log, sizeof f.data - log.pos, first, sizeof first);
    CHECK(first[0] == 0 && first[4] == 0, "ring order doesn't start with the oldest record");
    CHECK(datalog_oldest_record_offset(&log) == sizeof f.data - log.pos, "oldest at %u, want %u",
          (unsigned)datalog_oldest_record_offset(&log), (unsigned)(sizeof f.data - log.pos));
}
