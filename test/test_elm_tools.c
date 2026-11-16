#include "check.h"
#include "obd/sensors.h"
#include "obd/sniff.h"

// format_ram_scan_answer prints a positive answer to a RAM scan read as the ELM
// shows it: ISO-TP frames from 7E8, a float at byte offset at.
static void format_ram_scan_answer(char *out, size_t size, float value, int at) {
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

void test_elm_scan(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    CHECK(d.ready == 1, "polling didn't start after the probes");
    elm_request_ram_scan(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") == 0, "a scan asked for sent %s, want ATSH7E0", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 3);
    CHECK(strcmp(last_sent(&d), "2314FFF800003C\r") == 0, "first scan read %s", last_sent(&d));
    CHECK(strstr(s.status, "DEEP SCAN") == s.status, "status during the scan: %s", s.status);
    float pct = 25 * (2631 * ECU_ADC_STEP_VOLTS) - 12.5f; // 67.79%, as in the Alientech log
    char text[512];
    format_ram_scan_answer(text, sizeof text, pct, 8);
    feed_dongle_answer(&e, text, 4);
    CHECK(strcmp(last_sent(&d), "2314FFF8003C3C\r") == 0, "second scan read %s", last_sent(&d));
    format_ram_scan_answer(text, sizeof text, 67.8f, 0); // off the ADC grid
    feed_dongle_answer(&e, text, 5);
    feed_dongle_answer(&e, "7E8037F2331\r>", 6); // out of range: the end of the RAM
    CHECK(strcmp(last_sent(&d), "ATSH7DF\r") == 0, "after the scan sent %s, want ATSH7DF", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 7);
    CHECK(d.ready == 2 && e.phase == ELM_POLL, "polling didn't resume after the scan");
    CHECK(strstr(d.logged, "scan: FFF80008 = 67.792 (ethanol %?)") != NULL, "candidate not logged:\n%s", d.logged);
    CHECK(strstr(d.logged, "1 candidates") != NULL, "67.8 (off the grid) counted as a candidate:\n%s", d.logged);
}

// multi_answer answers MULTI_POLL: RPM 1726, lambda 1.0 (AFR 14.7) and
// its sensor's voltage, coolant 83°C, intake 31°C, throttle 20%, 15
// bytes over three frames.
static const char *multi_answer = "7E8100F410C1AF82480\r"
                                  "7E821006666057B0F47\r"
                                  "7E8221133AAAAAAAAAA\r>";

void test_elm_multi_pid(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 6; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    CHECK(strcmp(last_sent(&d), "010C24050F113\r") == 0, "last probe %s, want the multi-PID one", last_sent(&d));
    feed_dongle_answer(&e, multi_answer, 3);
    CHECK(e.multi_pid, "a full multi-PID answer didn't turn multi-PID polling on:\n%s", d.logged);
    feed_dongle_answer(&e, "12.4V\r>", 4); // ATRV, the first poll
    CHECK(strcmp(last_sent(&d), "010C24050F113\r") == 0, "polled %s, want every PID at once", last_sent(&d));
    feed_dongle_answer(&e, multi_answer, 5);
    CHECK(nearly_equal(s.readings[SIG_RPM].value, 1726) && nearly_equal(s.readings[SIG_AFR].value, 14.7f) &&
              nearly_equal(s.readings[SIG_COOLANT].value, 83) && nearly_equal(s.readings[SIG_IAT].value, 31) &&
              nearly_equal(s.readings[SIG_THROTTLE].value, 20),
          "multi-PID answer gave rpm %f afr %f coolant %f intake %f", (double)s.readings[SIG_RPM].value,
          (double)s.readings[SIG_AFR].value, (double)s.readings[SIG_COOLANT].value, (double)s.readings[SIG_IAT].value);
    feed_dongle_answer(&e, "?\r>", 6); // a dongle without the count digit
    CHECK(strcmp(last_sent(&d), "010C24050F11\r") == 0, "after ? sent %s, want 010C24050F11", last_sent(&d));
}

// answer_listen_window answers one window of a listen: the ATCF, then the
// monitor, frames printed until the dwell is over and the stop.
static void answer_listen_window(elm_t *e, uint32_t *t, const char *frames) {
    feed_dongle_answer(e, "OK\r>", *t);
    feed_dongle_answer(e, frames, *t);
    *t += ELM_LISTEN_DWELL_MS;
    elm_check_timers(e, *t);
    feed_dongle_answer(e, "STOPPED\r>", *t + 1);
    *t += 2;
}

void test_elm_listen(void) {
    static fake_dongle_t d;
    memset(&d, 0, sizeof d);
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    elm_request_can_listen(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATCAF0\r") == 0, "listening sent %s, want ATCAF0", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 4);
    CHECK(strcmp(last_sent(&d), "ATCM700\r") == 0, "then %s, want ATCM700", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 5);
    CHECK(strstr(s.status, "LISTEN 1/5") && strstr(s.status, "click") && !e.waiting,
          "the first step doesn't wait for a click: status %s", s.status);

    // 1A0 is a rolling counter; 3B0 the mode alone; 2C0 a counter and,
    // in its next byte, the mode; 4D0 a temperature, a degree more from
    // step 4: it drifts, the steps back to "on" disagree.
    static const char *const mode_3b0[SNIFF_MODES] = {"AA", "AB", "BB"};
    static const char *const mode_2c0[SNIFF_MODES] = {"00", "10", "30"};
    uint32_t t = 10;
    for (int step = 0; step < SNIFF_STEPS; step++) {
        CHECK(elm_listen_click(&e, t), "step %d: the click wasn't taken", step + 1);
        CHECK(strcmp(last_sent(&d), "ATCF000\r") == 0, "step %d began with %s, want ATCF000", step + 1, last_sent(&d));
        sniff_mode_t mode = sniff_steps[step].mode;
        for (int w = 0; w < ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS; w++) {
            CHECK(w == 0 || !elm_listen_click(&e, t), "a click was taken while listening");
            char frames[96];
            snprintf(frames, sizeof frames, "1A0%02X\r3B000%s\r2C0%02X%s\r4D0%02X\rNO DATA\r", w & 0x0F, mode_3b0[mode],
                     w & 0x0F, mode_2c0[mode], 0x50 + (step >= 3));
            answer_listen_window(&e, &t, frames);
        }
    }
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0, "after listening sent %s, want ATZ (session restart)", last_sent(&d));
    CHECK(strstr(d.logged, "sniff: 3B0 bits 0011: on 00AA trc 00AB off 00BB") != NULL,
          "3B0 not reported as a candidate:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 2C0 bits 0030: on 0F00 trc 0F10 off 0F30") != NULL,
          "2C0's flag, beside a counter, not reported:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 1A0") == NULL, "1A0, a counter, reported:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 4D0") == NULL, "4D0, drifting, reported:\n%s", d.logged);
}
