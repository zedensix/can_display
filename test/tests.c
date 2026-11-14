// Host-side tests for everything under src/app: no hardware, built with
// the address and undefined-behavior sanitizers.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elm327.h"
#include "flashlog.h"
#include "obd.h"
#include "screen.h"
#include "sensors.h"
#include "settings.h"
#include "sniff.h"
#include "ui.h"

static int failures;

#define CHECK(cond, ...)                                                                                               \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            failures++;                                                                                                \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                                \
            printf(__VA_ARGS__);                                                                                       \
            printf("\n");                                                                                              \
        }                                                                                                              \
    } while (0)

static bool near(float a, float b) {
    return fabsf(a - b) < 0.01f;
}

static void test_obd_decode(void) {
    state_t s = {0};
    can_frame_t rpm = {OBD_RESPONSE_ECU1, 8, {0x04, 0x41, 0x0C, 0x1A, 0xF8}};
    CHECK(obd_decode(&rpm, &s, 1) && near(s.readings[SIG_RPM].value, 1726), "rpm = %f, want 1726",
          (double)s.readings[SIG_RPM].value);
    can_frame_t coolant = {OBD_RESPONSE_ECU1, 8, {0x03, 0x41, 0x05, 0x7B}};
    CHECK(obd_decode(&coolant, &s, 1) && near(s.readings[SIG_COOLANT].value, 83), "coolant = %f, want 83",
          (double)s.readings[SIG_COOLANT].value);
    can_frame_t lambda = {OBD_RESPONSE_ECU1, 8, {0x06, 0x41, 0x24, 0x80, 0x00, 0x00, 0x00}};
    CHECK(obd_decode(&lambda, &s, 1) && near(s.readings[SIG_AFR].value, 14.7f), "afr = %f, want 14.7",
          (double)s.readings[SIG_AFR].value);
    can_frame_t other_ecu = {0x7E9, 8, {0x03, 0x41, 0x05, 0x7B}};
    CHECK(!obd_decode(&other_ecu, &s, 1), "a frame from 0x7E9 was decoded");
    can_frame_t multi = {OBD_RESPONSE_ECU1, 8, {0x10, 0x14, 0x49, 0x02}};
    CHECK(!obd_decode(&multi, &s, 1), "an ISO-TP first frame was decoded");
}

static void test_gt86_decode(void) {
    state_t s = {0};
    can_frame_t temps = {GT86_ID_TEMPERATURES, 8, {0, 0, 130, 125}};
    CHECK(gt86_decode(&temps, &s, 1) && near(s.readings[SIG_OIL].value, 90) &&
              near(s.readings[SIG_COOLANT].value, 85),
          "0x360 decoded oil %f coolant %f, want 90 and 85", (double)s.readings[SIG_OIL].value,
          (double)s.readings[SIG_COOLANT].value);
    can_frame_t rpm = {GT86_ID_RPM, 8, {0, 0, 0xB8, 0xCB}};
    CHECK(gt86_decode(&rpm, &s, 1) && near(s.readings[SIG_RPM].value, 3000), "0x140 rpm = %f, want 3000",
          (double)s.readings[SIG_RPM].value);
}

static void test_elm_parsing(void) {
    can_frame_t f;
    CHECK(elm_parse_frame("7E8 03 41 05 7B", &f) && f.id == 0x7E8 && f.dlc == 4 && f.data[3] == 0x7B,
          "spaced frame line misparsed");
    CHECK(elm_parse_frame("7E80341057B", &f) && f.dlc == 4, "compact frame line misparsed");
    CHECK(!elm_parse_frame("NO DATA", &f), "NO DATA parsed as a frame");
    CHECK(!elm_parse_frame("18DAF110034105", &f), "29bit frame parsed as 11bit");
    float v;
    CHECK(elm_parse_voltage("12.6V", &v) && near(v, 12.6f), "12.6V misparsed");
    CHECK(!elm_parse_voltage("?", &v), "? parsed as a voltage");

    char raw[] = "ATZ\r\r\rELM327 v2.3\r\rSEARCHING...\r 7E8 03 41 05 7B \r";
    char *lines[ELM_MAX_LINES];
    int n = elm_split_lines(raw, "ATZ", lines, ELM_MAX_LINES);
    CHECK(n == 2 && strcmp(lines[0], "ELM327 v2.3") == 0 && strcmp(lines[1], "7E8 03 41 05 7B") == 0,
          "split gave %d lines", n);
}

// fake_dongle records what the session sends and lets a test answer.
typedef struct {
    char sent[512][16];
    int sent_count;
    int ready, failed;
    char failed_step[32];
    char logged[4096];
} fake_dongle_t;

static void fake_write(void *ctx, const char *command) {
    fake_dongle_t *d = ctx;
    if (d->sent_count < 512) {
        snprintf(d->sent[d->sent_count++], 16, "%s", command);
    }
}
static void fake_log(void *ctx, const char *line) {
    fake_dongle_t *d = ctx;
    size_t len = strlen(d->logged);
    snprintf(d->logged + len, sizeof d->logged - len, "%s\n", line);
}
static void fake_ready(void *ctx) {
    ((fake_dongle_t *)ctx)->ready++;
}
static void fake_failed(void *ctx, const char *step, const char *error) {
    fake_dongle_t *d = ctx;
    d->failed++;
    snprintf(d->failed_step, sizeof d->failed_step, "%s", step);
    (void)error;
}

static void answer(elm_t *e, const char *text, uint32_t now) {
    elm_receive(e, (const uint8_t *)text, strlen(text), now);
}

static const char *last_sent(fake_dongle_t *d) {
    return d->sent[d->sent_count - 1];
}

static void test_elm_session(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_callbacks_t cb = {fake_write, fake_log, fake_ready, fake_failed, &d};
    elm_start(&e, &s, cb, 0);
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0, "first command %s, want ATZ", last_sent(&d));
    answer(&e, "ATZ\r\rELM327 v2.3\r\r>", 10);
    for (int i = 0; i < 7; i++) {
        answer(&e, "OK\r\r>", 20);
    }
    CHECK(strcmp(last_sent(&d), "ATI\r") == 0, "after init sent %s, want the ATI probe", last_sent(&d));
    for (int i = 0; i < 7; i++) {
        answer(&e, "whatever\r>", 30);
    }
    CHECK(d.ready == 1, "ready called %d times after the probes", d.ready);
    CHECK(strcmp(last_sent(&d), "ATRV\r") == 0, "first poll %s, want ATRV", last_sent(&d));
    answer(&e, "12.4V\r>", 40);
    CHECK(near(s.readings[SIG_BATTERY].value, 12.4f), "battery %f, want 12.4", (double)s.readings[SIG_BATTERY].value);
    CHECK(strcmp(last_sent(&d), "010C1\r") == 0, "then %s, want 010C1", last_sent(&d));
    answer(&e, "?\r>", 50);
    CHECK(strcmp(last_sent(&d), "010C\r") == 0, "after ? sent %s, want 010C without the count digit",
          last_sent(&d));
    answer(&e, "7E804410C1AF8\r>", 60);
    CHECK(near(s.readings[SIG_RPM].value, 1726), "rpm %f, want 1726", (double)s.readings[SIG_RPM].value);
    CHECK(e.ecu_ms == 60, "ecu_ms %u after the ECU answered at 60, want 60 (car-off detection)", (unsigned)e.ecu_ms);
    CHECK(strcmp(last_sent(&d), "0124\r") == 0, "then %s, want 0124", last_sent(&d));

    int before = d.sent_count;
    elm_tick(&e, 60 + ELM_TIMEOUT_MS + 1);
    CHECK(d.sent_count == before + 1 && strcmp(last_sent(&d), "0124\r") == 0, "a timeout didn't resend 0124");
    elm_tick(&e, 60 + 2 * ELM_TIMEOUT_MS + 2);
    elm_tick(&e, 60 + 3 * ELM_TIMEOUT_MS + 3);
    CHECK(d.failed == 1 && strcmp(d.failed_step, "OBD poll") == 0, "after %d retries: failed %d, step %s",
          ELM_RETRIES, d.failed, d.failed_step);
}

// oil_answer is a 2101 answer: 0x2B bytes in a first frame and 6
// consecutive frames, the oil byte (AC) holding 130, i.e. 90°C.
static const char *oil_answer = "7E8102B610100000000\r"
                                "7E82100000000000000\r"
                                "7E82200000000000000\r"
                                "7E82300000000000000\r"
                                "7E82400000082000000\r"
                                "7E82500000000000000\r"
                                "7E82600000000000000\r>";

static void test_elm_oil(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        answer(&e, "OK\r>", 2);
    }
    answer(&e, "12.4V\r>", 3); // ATRV, the first poll
    answer(&e, "NO DATA\r>", ELM_OIL_INTERVAL_MS + 3);
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") == 0, "after %dms sent %s, want the oil sequence's ATSH7E0",
          ELM_OIL_INTERVAL_MS, last_sent(&d));
    answer(&e, "OK\r>", ELM_OIL_INTERVAL_MS + 4);
    CHECK(strcmp(last_sent(&d), "2101\r") == 0, "then %s, want 2101", last_sent(&d));
    answer(&e, oil_answer, ELM_OIL_INTERVAL_MS + 5);
    CHECK(near(s.readings[SIG_OIL].value, 90), "oil %f, want 90", (double)s.readings[SIG_OIL].value);
    CHECK(strcmp(last_sent(&d), "ATSH7DF\r") == 0, "then %s, want the header back to ATSH7DF", last_sent(&d));

    for (int miss = 0; miss < ELM_OIL_MAX_MISSES; miss++) {
        uint32_t t = (uint32_t)(miss + 2) * (ELM_OIL_INTERVAL_MS + 10);
        answer(&e, "OK\r>", t);
        while (strcmp(last_sent(&d), "ATSH7E0\r") != 0) {
            answer(&e, "NO DATA\r>", t);
        }
        answer(&e, "OK\r>", t);
        answer(&e, "NO DATA\r>", t); // 2101 unanswered
    }
    int before = d.sent_count;
    for (int i = 0; i < 20; i++) {
        answer(&e, "OK\r>", 100000 + (uint32_t)i * ELM_OIL_INTERVAL_MS);
    }
    bool asked = false;
    for (int i = before; i < d.sent_count; i++) {
        asked = asked || strcmp(d.sent[i], "2101\r") == 0;
    }
    CHECK(!asked, "2101 still asked after %d answers without the oil temperature", ELM_OIL_MAX_MISSES);
}

// With the car off at boot nothing answers: 2101 must still be asked,
// so the oil temperature shows once the car starts.
static void test_elm_oil_car_off(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        answer(&e, "OK\r>", 2);
    }
    for (uint32_t t = 3; t < 10 * ELM_OIL_INTERVAL_MS; t += 100) {
        answer(&e, strcmp(last_sent(&d), "ATSH7E0\r") == 0 || strcmp(last_sent(&d), "ATSH7DF\r") == 0 ? "OK\r>"
                                                                                                    : "NO DATA\r>",
               t);
    }
    int asked = 0;
    for (int i = 0; i < d.sent_count; i++) {
        asked += strcmp(d.sent[i], "2101\r") == 0;
    }
    CHECK(asked > ELM_OIL_MAX_MISSES, "2101 asked %d times with the car off, want it kept", asked);
}

// scan_answer prints a positive answer to a RAM scan read as the ELM
// shows it: ISO-TP frames from 7E8, a float at byte offset at.
static void scan_answer(char *out, size_t size, float value, int at) {
    uint8_t msg[1 + ELM_SCAN_CHUNK] = {0x63};
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    for (int i = 0; i < 4; i++) {
        msg[1 + at + i] = (uint8_t)(bits >> (24 - 8 * i));
    }
    int len = (int)sizeof msg;
    size_t n = (size_t)snprintf(out, size, "7E810%02X", len);
    for (int i = 0; i < 6; i++) {
        n += (size_t)snprintf(out + n, size - n, "%02X", msg[i]);
    }
    for (int pos = 6, seq = 1; pos < len; pos += 7, seq++) {
        n += (size_t)snprintf(out + n, size - n, "\r7E82%X", seq % 16);
        for (int i = pos; i < pos + 7; i++) {
            n += (size_t)snprintf(out + n, size - n, "%02X", i < len ? msg[i] : 0);
        }
    }
    snprintf(out + n, size - n, "\r>");
}

static void test_elm_scan(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        answer(&e, "OK\r>", 2);
    }
    CHECK(d.ready == 1, "polling didn't start after the probes");
    elm_scan_ram(&e);
    answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") == 0, "a scan asked for sent %s, want ATSH7E0", last_sent(&d));
    answer(&e, "OK\r>", 3);
    CHECK(strcmp(last_sent(&d), "2314FFF800003C\r") == 0, "first scan read %s", last_sent(&d));
    CHECK(strstr(s.status, "DEEP SCAN") == s.status, "status during the scan: %s", s.status);
    float pct = 25 * (2631 * ECU_ADC_STEP_VOLTS) - 12.5f; // 67.79%, as in the Alientech log
    char text[512];
    scan_answer(text, sizeof text, pct, 8);
    answer(&e, text, 4);
    CHECK(strcmp(last_sent(&d), "2314FFF8003C3C\r") == 0, "second scan read %s", last_sent(&d));
    scan_answer(text, sizeof text, 67.8f, 0); // off the ADC grid
    answer(&e, text, 5);
    answer(&e, "7E8037F2331\r>", 6); // out of range: the end of the RAM
    CHECK(strcmp(last_sent(&d), "ATSH7DF\r") == 0, "after the scan sent %s, want ATSH7DF", last_sent(&d));
    answer(&e, "OK\r>", 7);
    CHECK(d.ready == 2 && e.phase == ELM_POLL, "polling didn't resume after the scan");
    CHECK(strstr(d.logged, "scan: FFF80008 = 67.792 (ethanol %?)") != NULL, "candidate not logged:\n%s", d.logged);
    CHECK(strstr(d.logged, "1 candidates") != NULL, "67.8 (off the grid) counted as a candidate:\n%s", d.logged);
}

// multi_answer answers MULTI_POLL: RPM 1726, lambda 1.0 (AFR 14.7) and
// its sensor's voltage, coolant 83°C, intake 31°C, 13 bytes over two
// frames.
static const char *multi_answer = "7E8100D410C1AF82480\r"
                                  "7E821006666057B0F47\r>";

static void test_elm_multi_pid(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 6; i++) {
        answer(&e, "OK\r>", 2);
    }
    CHECK(strcmp(last_sent(&d), "010C24050F1\r") == 0, "last probe %s, want the multi-PID one", last_sent(&d));
    answer(&e, multi_answer, 3);
    CHECK(e.multi_pid, "a full multi-PID answer didn't turn multi-PID polling on:\n%s", d.logged);
    answer(&e, "12.4V\r>", 4); // ATRV, the first poll
    CHECK(strcmp(last_sent(&d), "010C24050F1\r") == 0, "polled %s, want every PID at once", last_sent(&d));
    answer(&e, multi_answer, 5);
    CHECK(near(s.readings[SIG_RPM].value, 1726) && near(s.readings[SIG_AFR].value, 14.7f) &&
              near(s.readings[SIG_COOLANT].value, 83) && near(s.readings[SIG_IAT].value, 31),
          "multi-PID answer gave rpm %f afr %f coolant %f intake %f", (double)s.readings[SIG_RPM].value,
          (double)s.readings[SIG_AFR].value, (double)s.readings[SIG_COOLANT].value,
          (double)s.readings[SIG_IAT].value);
    answer(&e, "?\r>", 6); // a dongle without the count digit
    CHECK(strcmp(last_sent(&d), "010C24050F\r") == 0, "after ? sent %s, want 010C24050F", last_sent(&d));
}

// listen_window answers one window of a listen: the ATCF, then the
// monitor, frames printed until the dwell is over and the stop.
static void listen_window(elm_t *e, uint32_t *t, const char *frames) {
    answer(e, "OK\r>", *t);
    answer(e, frames, *t);
    *t += ELM_LISTEN_DWELL_MS;
    elm_tick(e, *t);
    answer(e, "STOPPED\r>", *t + 1);
    *t += 2;
}

static void test_elm_listen(void) {
    static fake_dongle_t d;
    memset(&d, 0, sizeof d);
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        answer(&e, "OK\r>", 2);
    }
    elm_listen_can(&e);
    answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATCAF0\r") == 0, "listening sent %s, want ATCAF0", last_sent(&d));
    answer(&e, "OK\r>", 4);
    CHECK(strcmp(last_sent(&d), "ATCM700\r") == 0, "then %s, want ATCM700", last_sent(&d));
    answer(&e, "OK\r>", 5);
    CHECK(strcmp(last_sent(&d), "ATCF000\r") == 0, "then %s, want ATCF000", last_sent(&d));
    answer(&e, "OK\r>", 6);
    CHECK(strcmp(last_sent(&d), "ATMA\r") == 0 && e.monitoring, "then %s, want ATMA", last_sent(&d));
    elm_tick(&e, 6 + ELM_LISTEN_DWELL_MS);
    CHECK(strcmp(last_sent(&d), "\r") == 0, "the dwell over, sent %s, want a stop", last_sent(&d));
    answer(&e, "STOPPED\r>", 6 + ELM_LISTEN_DWELL_MS);
    CHECK(strcmp(last_sent(&d), "ATCF100\r") == 0, "next window %s, want ATCF100", last_sent(&d));

    // 1A0 counts all along; 3B0 only changes with the presses.
    uint32_t t = 10 + ELM_LISTEN_DWELL_MS;
    for (int i = 0; t < ELM_LISTEN_MS + 3; i++) {
        char frames[64];
        bool pressed = t >= ELM_LISTEN_BASELINE_MS && i % 2;
        snprintf(frames, sizeof frames, "1A0%02X\r3B000%s\rNO DATA\r", i & 0xFF, pressed ? "AB" : "AA");
        listen_window(&e, &t, frames);
    }
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0, "after listening sent %s, want ATZ (session restart)", last_sent(&d));
    CHECK(strstr(d.logged, "sniff: 3B0 x") != NULL && strstr(d.logged, "00AA -> 00AB") != NULL,
          "3B0 not reported as a candidate:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 1A0 x") == NULL, "1A0, changing in the baseline, reported:\n%s", d.logged);
}

static void test_isotp(void) {
    isotp_t r = {0};
    can_frame_t first = {OBD_RESPONSE_ECU1, 8, {0x10, 0x0A, 0x61, 0x01, 1, 2, 3, 4}};
    can_frame_t next = {OBD_RESPONSE_ECU1, 8, {0x21, 5, 6, 7, 8, 9, 10, 11}};
    CHECK(!isotp_feed(&r, &first), "a first frame alone completed a message");
    CHECK(isotp_feed(&r, &next) && r.len == 10 && r.data[0] == 0x61 && r.data[9] == 8,
          "first + consecutive gave len %d", r.len);
    can_frame_t wrong_sequence = {OBD_RESPONSE_ECU1, 8, {0x22, 0, 0, 0, 0, 0, 0, 0}};
    isotp_feed(&r, &first);
    CHECK(!isotp_feed(&r, &wrong_sequence), "a consecutive frame out of sequence was accepted");
}

static void test_elm_init_rejected(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    answer(&e, "ELM327 v1.5\r>", 1);
    answer(&e, "?\r>", 2);
    CHECK(d.failed == 1 && strcmp(d.failed_step, "ELM327 init") == 0, "a rejected ATE0 gave failed %d, step %s",
          d.failed, d.failed_step);
}

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
static void collect(void *ctx, const uint8_t *data, size_t len) {
    (void)ctx;
    memcpy(dumped + dumped_len, data, len);
    dumped_len += len;
    dumped[dumped_len] = 0;
}
static const char *dump(flashlog_t *log) {
    dumped_len = 0;
    dumped[0] = 0;
    flashlog_dump(log, collect, NULL);
    return dumped;
}

static void test_flashlog(void) {
    static fake_flash_t flash;
    memset(flash.data, 0xFF, sizeof flash.data);
    flash_dev_t dev = {&flash, sizeof flash.data, flash_read, flash_program, flash_erase};
    flashlog_t log;
    CHECK(flashlog_open(&log, dev), "open failed");
    flashlog_append(&log, "boot");
    flashlog_append(&log, "ble: connected");
    flashlog_open(&log, dev);
    flashlog_append(&log, "boot again");
    CHECK(strcmp(dump(&log), "boot\nble: connected\nboot again\n") == 0, "after a reopen the log is %s", dumped);

    for (int i = 0; i < 3000; i++) {
        char line[16];
        snprintf(line, sizeof line, "line %d", i);
        flashlog_append(&log, line);
        if (i == 1500) {
            flashlog_open(&log, dev);
        }
    }
    dump(&log);
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
    CHECK(strcmp(dump(&log), "boot\n") == 0, "foreign data wasn't erased: %s", dumped);
}

static void test_settings(void) {
    settings_t cfg = {.brightness = 40, .auto_connect = true, .flip = true, .ethanol_sensor = true, .shift_rpm = 7100};
    uint8_t buf[SETTINGS_SIZE];
    settings_encode(cfg, buf);
    CHECK(settings_equal(settings_decode(buf), cfg), "settings didn't round-trip");
    buf[4] = 1; // saved by a firmware without the shift light
    settings_t old = settings_decode(buf);
    CHECK(old.brightness == 40 && old.flip && old.shift_rpm == 0, "version 1 settings lost (shift %d)", old.shift_rpm);
    memset(buf, 0xFF, sizeof buf);
    CHECK(settings_equal(settings_decode(buf), SETTINGS_DEFAULT), "erased flash didn't give the defaults");
}

static void test_screen(void) {
    screen_t s = {0};
    settings_t cfg = SETTINGS_DEFAULT;
    screen_turn(&s, 2, 6, &cfg);
    CHECK(s.view == 2, "view %d after 2 detents, want 2", s.view);
    screen_turn(&s, -3, 6, &cfg);
    CHECK(s.view == 6, "view %d going back past the grid, want the last gauge", s.view);
    screen_turn(&s, 1, 6, &cfg);
    CHECK(s.view == 0, "view %d after the last gauge, want the grid", s.view);

    screen_press(&s, &cfg);
    CHECK(s.page == PAGE_SETTINGS && s.cursor == ROW_BRIGHTNESS, "a click didn't open the settings");
    screen_turn(&s, -1, 6, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MAX, "turning without editing changed the brightness");
    screen_press(&s, &cfg);
    screen_turn(&s, -3, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MAX - 3 * BRIGHTNESS_STEP && !s.editing, "brightness %d, want 3 steps down",
          cfg.brightness);
    screen_press(&s, &cfg);
    screen_turn(&s, -50, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MIN, "brightness %d, want it stopped at the minimum", cfg.brightness);
    screen_turn(&s, 1, 6, &cfg);
    screen_press(&s, &cfg);
    screen_turn(&s, 1, 6, &cfg);
    CHECK(cfg.shift_rpm == SHIFT_RPM_MIN, "shift light %d turned up from off, want %d", cfg.shift_rpm, SHIFT_RPM_MIN);
    screen_turn(&s, 50, 6, &cfg);
    CHECK(cfg.shift_rpm == SHIFT_RPM_MAX, "shift light %d, want it stopped at %d", cfg.shift_rpm, SHIFT_RPM_MAX);
    screen_turn(&s, -100, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.shift_rpm == 0, "shift light %d turned all the way down, want off", cfg.shift_rpm);
    screen_turn(&s, 1, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.auto_connect, "a click on auto-connect didn't turn it on");
    screen_turn(&s, 1, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.flip, "a click on flip didn't turn it on");
    screen_turn(&s, 1, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(cfg.ethanol_sensor, "a click on the ethanol sensor didn't turn it on");
    screen_turn(&s, 1, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(s.scan_requested && s.page == PAGE_GAUGES, "a click on Scan ECU RAM didn't ask for a scan");
    screen_press(&s, &cfg);
    screen_turn(&s, ROW_LISTEN, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(s.listen_requested && s.page == PAGE_GAUGES, "a click on Listen CAN didn't ask to listen");
    screen_press(&s, &cfg);
    screen_turn(&s, ROW_DISCONNECT, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(s.disconnect_requested && s.page == PAGE_GAUGES, "a click on Disconnect didn't ask to disconnect");
    screen_press(&s, &cfg);
    screen_turn(&s, 10, 6, &cfg);
    screen_press(&s, &cfg);
    CHECK(s.page == PAGE_GAUGES, "a click on Back didn't return to the gauges");
}

static void test_sensors(void) {
    float at_pin = ETHANOL_SENSOR_MAX_VOLTS * divider_ratio(ETHANOL_DIVIDER);
    CHECK(at_pin < 3.3f, "the divider lets %fV through at the sensor's max, over the RP2040's 3.3V", (double)at_pin);
    CHECK(near(divider_undo(ETHANOL_DIVIDER, at_pin), ETHANOL_SENSOR_MAX_VOLTS), "divider_undo isn't the inverse");
    CHECK(near(sensors_ethanol_percent(2.5f), 50), "2.5V gave %f%%, want 50", (double)sensors_ethanol_percent(2.5f));
    CHECK(sensors_ethanol_percent(0) == 0 && sensors_ethanol_percent(5) == 100, "ethanol %% isn't clamped");
    CHECK(sensors_on_ecu_adc_grid(1234 * ECU_ADC_STEP_VOLTS), "a whole number of ADC steps is off the grid");
    CHECK(!sensors_on_ecu_adc_grid(1234.5f * ECU_ADC_STEP_VOLTS), "half an ADC step is on the grid");
}

static void test_extremes(void) {
    state_t s = {0};
    state_set(&s, SIG_AFR, 14.7f, 1);
    state_set(&s, SIG_AFR, 11.9f, 2);
    state_set(&s, SIG_AFR, 16.2f, 3);
    state_set(&s, SIG_AFR, 14.1f, 4);
    reading_t r = s.readings[SIG_AFR];
    CHECK(near(r.value, 14.1f) && near(r.min, 11.9f) && near(r.max, 16.2f), "afr %f, min %f, max %f",
          (double)r.value, (double)r.min, (double)r.max);
}

static void test_wrap(void) {
    const char *text = "BLE connect failed: no known ELM327 serial service on this device";
    char lines[8][64];
    int n = ui_wrap(&font_bold9, text, 216, lines, 8);
    CHECK(n >= 2, "wrap gave %d line(s), want the long error split", n);
    char joined[256] = "";
    for (int i = 0; i < n; i++) {
        CHECK(gfx_text_width(&font_bold9, lines[i]) <= 216, "line %d is too wide", i);
        strcat(joined, i ? " " : "");
        strcat(joined, lines[i]);
    }
    CHECK(strcmp(joined, text) == 0, "rejoined lines = %s", joined);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_obd_decode();
    test_gt86_decode();
    test_elm_parsing();
    test_elm_session();
    test_elm_init_rejected();
    test_elm_oil();
    test_elm_oil_car_off();
    test_elm_scan();
    test_elm_multi_pid();
    test_elm_listen();
    test_isotp();
    test_flashlog();
    test_settings();
    test_screen();
    test_sensors();
    test_extremes();
    test_wrap();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
