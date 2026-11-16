#include "check.h"
#include "obd/sensors.h"

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
    for (int i = 0; i < 7 + 9; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    CHECK(d.ready == 1, "polling didn't start after the probes");
    elm_request_ram_scan(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") == 0, "a scan asked for sent %s, want ATSH7E0", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 3);
    CHECK(strcmp(last_sent(&d), "2314FFF800003C\r") == 0, "first scan read %s", last_sent(&d));
    CHECK(strstr(s.status, "DEEP SCAN") == s.status && s.wait == WAIT_CANCEL, "status during the scan: %s, wait %d",
          s.status, s.wait);
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
    CHECK(s.wait == WAIT_NONE, "the scan's Cancel still offered after it");

    // Another, cancelled while its first read is in flight: that answer ends it.
    CHECK(!elm_cancel_ram_scan(&e), "a cancel taken with no scan running");
    elm_request_ram_scan(&e);
    feed_dongle_answer(&e, "12.4V\r>", 8);
    feed_dongle_answer(&e, "OK\r>", 9); // ATSH7E0
    CHECK(elm_cancel_ram_scan(&e) && s.wait == WAIT_NONE, "a running scan's cancel wasn't taken");
    format_ram_scan_answer(text, sizeof text, pct, 8);
    feed_dongle_answer(&e, text, 10);
    CHECK(strcmp(last_sent(&d), "ATSH7DF\r") == 0, "after a cancel sent %s, want ATSH7DF", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 11);
    CHECK(d.ready == 3 && e.phase == ELM_POLL, "polling didn't resume after a cancel");
}

// fast_answer answers MULTI_POLL_FAST: RPM 1726, lambda 1.0 (AFR 14.7)
// and its sensor's voltage, throttle 20%, pedal 50.2%, load 40%, timing
// 8°, 17 bytes over three frames. slow_answer, MULTI_POLL_SLOW: coolant
// 83°C, intake 31°C, 100 km/h, commanded lambda 1.0, rail 20 MPa, short
// term trim +7.8%, 15 bytes.
static const char *fast_answer = "7E81011410C1AF82480\r"
                                 "7E82100666611335A80\r"
                                 "7E82204660E90AAAAAA\r>";
static const char *slow_answer = "7E8100F41057B0F470D\r"
                                 "7E821644480002307D0\r"
                                 "7E822068AAAAAAAAAAA\r>";

void test_elm_multi_pid(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) { // up to the fast multi-PID probe
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    CHECK(strcmp(last_sent(&d), "010C24115A040E3\r") == 0, "probe %s, want the fast multi-PID one", last_sent(&d));
    feed_dongle_answer(&e, fast_answer, 3);
    CHECK(strcmp(last_sent(&d), "01050F0D4423063\r") == 0, "probe %s, want the slow multi-PID one", last_sent(&d));
    feed_dongle_answer(&e, slow_answer, 3);
    CHECK(e.multi_pid, "full multi-PID answers didn't turn multi-PID polling on:\n%s", d.logged);
    feed_dongle_answer(&e, "12.4V\r>", 4); // ATRV, the first poll
    CHECK(strcmp(last_sent(&d), "010C24115A040E3\r") == 0, "polled %s, want the fast PIDs", last_sent(&d));
    feed_dongle_answer(&e, fast_answer, 5);
    CHECK(strcmp(last_sent(&d), "01050F0D4423063\r") == 0, "polled %s, want the slow PIDs", last_sent(&d));
    feed_dongle_answer(&e, slow_answer, 5);
    static const struct {
        signal_t sig;
        float want;
    } got[] = {{SIG_RPM, 1726},  {SIG_AFR, 14.7f}, {SIG_THROTTLE, 20},      {SIG_PEDAL, 50.2f},
               {SIG_LOAD, 40},   {SIG_TIMING, 8},  {SIG_COOLANT, 83},       {SIG_IAT, 31},
               {SIG_SPEED, 100}, {SIG_RAIL, 20},   {SIG_AFR_TARGET, 14.7f}, {SIG_STFT, 7.8125f}};
    for (size_t i = 0; i < sizeof got / sizeof got[0]; i++) {
        CHECK(fabsf(s.readings[got[i].sig].value - got[i].want) < 0.1f, "signal %d read %f, want %f", got[i].sig,
              (double)s.readings[got[i].sig].value, (double)got[i].want);
    }
    CHECK(strcmp(last_sent(&d), "010C24115A040E3\r") == 0, "then %s, want the fast PIDs again", last_sent(&d));
    feed_dongle_answer(&e, "?\r>", 6); // a dongle without the count digit
    CHECK(strcmp(last_sent(&d), "010C24115A040E\r") == 0, "after ? sent %s, want 010C24115A040E", last_sent(&d));
}

// The drive mode through the dongle: every ELM_MODE_INTERVAL_MS polling
// stops for a raw monitor of 0D3 alone, which stops at its first frame,
// then the dongle is set back.
void test_elm_drive_mode(void) {
    static fake_dongle_t d;
    memset(&d, 0, sizeof d);
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 9; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    // Polling (the oil and ethanol reads included) until the mode's turn.
    uint32_t t = 2 + ELM_MODE_INTERVAL_MS;
    for (int i = 0; i < 40 && strcmp(last_sent(&d), "ATCAF0\r") != 0; i++) {
        feed_dongle_answer(&e, strncmp(last_sent(&d), "AT", 2) == 0 ? "OK\r>" : "NO DATA\r>", t);
    }
    CHECK(strcmp(last_sent(&d), "ATCAF0\r") == 0, "the mode's monitor didn't start: last sent %s", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", t);
    CHECK(strcmp(last_sent(&d), "ATCRA0D3\r") == 0, "then %s, want ATCRA0D3", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", t);
    CHECK(strcmp(last_sent(&d), "ATMA\r") == 0 && e.monitoring, "then %s, want ATMA", last_sent(&d));
    feed_dongle_answer(&e, "0D3200EC00E00008D\r", t + 30); // everything off
    CHECK(strcmp(last_sent(&d), "\r") == 0, "the monitor didn't stop at the frame: last sent %s", last_sent(&d));
    CHECK(state_drive_mode(&s, t + 30) == DRIVE_VSC_OFF, "the mode read is %d, want VSC off",
          state_drive_mode(&s, t + 30));
    feed_dongle_answer(&e, "STOPPED\r>", t + 40);
    CHECK(strcmp(last_sent(&d), "ATCRA\r") == 0, "after the monitor %s, want ATCRA", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", t + 50);
    CHECK(strcmp(last_sent(&d), "ATCAF1\r") == 0, "then %s, want ATCAF1", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", t + 60);
    CHECK(strncmp(last_sent(&d), "01", 2) == 0 || strcmp(last_sent(&d), "ATRV\r") == 0, "polling didn't resume: %s",
          last_sent(&d));
}
