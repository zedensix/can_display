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
    settings_t cfg = {.brightness = 40, .flip = true, .landscape = true, .shift_rpm = 7100};
    uint8_t buf[SETTINGS_SIZE];
    settings_encode(cfg, buf);
    CHECK(settings_equal(settings_decode(buf), cfg), "settings didn't round-trip");
    buf[4] = 5; // saved with a BLE dongle, auto-connect and the CAN source's flag
    buf[6] |= 1 << 0 | 1 << 4 | 1 << 5;
    CHECK(settings_equal(settings_decode(buf), cfg), "version 5 settings misread");
    buf[4] = 1; // saved by a firmware without the shift light
    settings_t old = settings_decode(buf);
    CHECK(old.brightness == 40 && old.flip && old.shift_rpm == 0, "version 1 settings lost (shift %d)", old.shift_rpm);
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
    state_set_reading(&s, SIG_THROTTLE, 42.5f, 0);
    for (uint32_t t = 0; t < 11 * DATALOG_SAMPLE_MS; t += DATALOG_SAMPLE_MS) {
        s.readings[SIG_RPM].updated_ms = s.readings[SIG_AFR].updated_ms = s.readings[SIG_THROTTLE].updated_ms = t;
        datalog_sample(&log, &s, t);
    }
    datalog_flush(&log);
    // 11 records: 4 a page, the last 3 flushed in the third.
    const int R = DATALOG_RECORD;
    CHECK(f.data[8] == (3000 & 0xFF) && f.data[9] == 3000 >> 8, "the first sample's RPM isn't in flash");
    CHECK(f.data[22] == (425 & 0xFF) && f.data[23] == 425 >> 8, "the throttle (x10) isn't at bytes 22-23");
    CHECK(f.data[24] == 0x00 && f.data[25] == 0x80, "a pedal never read isn't -32768");
    CHECK(f.data[42] == 0xFF && f.data[63] == 0xFF, "bytes 42 to 63, free, aren't left erased");
    CHECK(f.data[10 * R + 2] == DATALOG_SAMPLE && f.data[10 * R + 4] == (10 * DATALOG_SAMPLE_MS & 0xFF),
          "the 11th sample isn't in flash");
    CHECK(f.data[8 + 2 * 1] == 0x00 && f.data[8 + 2 * 1 + 1] == 0x80, "a coolant never read isn't -32768");

    // A new run carries on after it, in a new session.
    datalog_open(&log, dev);
    CHECK(log.pos == 11 * (uint32_t)R && log.session == 1, "reopened: pos %u session %u, want %d and 1",
          (unsigned)log.pos, log.session, 11 * R);
    datalog_sample(&log, &s, 0);
    datalog_flush(&log);
    CHECK(f.data[11 * R] == 1 && f.data[10 * R] == 0, "session 1 didn't follow session 0");

    // Read back in ring order, the oldest first.
    uint8_t first[DATALOG_RECORD];
    datalog_read(&log, sizeof f.data - log.pos, first, sizeof first);
    CHECK(first[0] == 0 && first[4] == 0, "ring order doesn't start with the oldest record");
    CHECK(datalog_oldest_record_offset(&log) == sizeof f.data - log.pos, "oldest at %u, want %u",
          (unsigned)datalog_oldest_record_offset(&log), (unsigned)(sizeof f.data - log.pos));

    // A ring an older firmware wrote: 3 records of 32 bytes, session 5.
    // The new ones go after them, at the next 64-byte boundary.
    memset(f.data, 0xFF, sizeof f.data);
    for (int i = 0; i < 3; i++) {
        uint8_t *r = &f.data[i * DATALOG_RECORD_V1];
        r[0] = 5, r[1] = 0, r[2] = DATALOG_SAMPLE_V1, r[4] = (uint8_t)i;
        r[5] = r[6] = r[7] = 0;
    }
    datalog_open(&log, dev);
    CHECK(log.pos == 2 * (uint32_t)R && log.session == 6, "after old records: pos %u session %u, want %d and 6",
          (unsigned)log.pos, log.session, 2 * R);
    datalog_sample(&log, &s, 0);
    datalog_flush(&log);
    CHECK(f.data[2 * DATALOG_RECORD_V1] == 5 && f.data[2 * R] == 6, "an old record was overwritten, or misplaced");
    datalog_open(&log, dev);
    CHECK(log.pos == 3 * (uint32_t)R && log.session == 7, "reopened after both kinds: pos %u session %u",
          (unsigned)log.pos, log.session);

    // What's new since a pull: everything first, then only what followed.
    uint32_t offset, len;
    uint32_t at = datalog_since(&log, DATALOG_FROM_OLDEST, &offset, &len);
    CHECK(at == log.pos && offset == datalog_oldest_record_offset(&log) && offset + len == sizeof f.data,
          "from the oldest: at %u, offset %u, len %u", (unsigned)at, (unsigned)offset, (unsigned)len);
    datalog_since(&log, at, &offset, &len);
    CHECK(len == 0, "nothing new, yet %u bytes", (unsigned)len);
    datalog_sample(&log, &s, 0);
    datalog_sample(&log, &s, DATALOG_SAMPLE_MS);
    datalog_flush(&log);
    uint32_t next = datalog_since(&log, at, &offset, &len);
    uint8_t got[DATALOG_RECORD];
    datalog_read(&log, offset, got, sizeof got);
    CHECK(len == 2 * (uint32_t)R && next == at + 2 * R && got[0] == 7 && got[4] == 0,
          "2 records since: len %u, next %u, session %u", (unsigned)len, (unsigned)next, got[0]);
    // Round the ring's end: the pull from near it carries on from its start.
    while (log.pos != 0) {
        datalog_sample(&log, &s, 0);
    }
    at = datalog_since(&log, DATALOG_FROM_OLDEST, &offset, &len);
    for (int i = 0; i < 4; i++) {
        datalog_sample(&log, &s, 1000 + (uint32_t)i);
    }
    datalog_flush(&log);
    datalog_since(&log, at, &offset, &len);
    datalog_read(&log, offset, got, sizeof got);
    CHECK(at == 0 && len == 4 * (uint32_t)R && got[4] == (1000 & 0xFF), "across the end: from %u, len %u", (unsigned)at,
          (unsigned)len);
}
