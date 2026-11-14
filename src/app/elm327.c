#include "elm327.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sensors.h"

static const char *const init_commands[] = {
    "ATZ",    // reset; answers with its banner, not OK
    "ATE0",   // echo off
    "ATL0",   // no linefeeds after CR
    "ATS0",   // no spaces between hex bytes
    "ATH1",   // print CAN IDs: needed to rebuild frames
    "ATCAF1", // automatic ISO-TP formatting
    // Protocol 6 = ISO 15765-4 CAN, 11bit IDs, 500kbps: what the GT86
    // speaks on its OBD port. Fixing it skips the slow auto-detect. If
    // the dongle refuses to talk to the car, try ATSP0 (auto).
    "ATSP6",
    "ATSH7DF", // requests to the functional address
};
#define INIT_COUNT (int)(sizeof init_commands / sizeof init_commands[0])

// The raw answers to these are logged: the first thing to look at when
// the dash stays at "--". 0100, 0120 and 0140 are the bitmaps of the
// PIDs the engine ECU supports. MULTI_POLL asks for every polled PID at
// once: if its answer holds them all, polling uses it (one round trip
// over BLE instead of four), with the response count digit like 010C1.
#define MULTI_POLL "010C24050F"
static const char *const probe_commands[] = {"ATI", "ATRV", "0100", "0120", "0140", "010C", MULTI_POLL "1"};
#define PROBE_COUNT (int)(sizeof probe_commands / sizeof probe_commands[0])

// RPM comes every other request: the shift light follows it.
static const uint8_t poll_pids[] = {PID_RPM, PID_LAMBDA, PID_RPM, PID_COOLANT, PID_RPM, PID_IAT};
#define POLL_COUNT (int)(sizeof poll_pids / sizeof poll_pids[0])

// The oil temperature (see obd.h) is asked to the engine ECU directly,
// which takes switching the header there and back. It changes slowly:
// every few seconds is plenty. After a few answers without it while the
// ECU answers the rest, the session stops asking.
static const char *const oil_commands[] = {"ATSH7E0", "2101", "ATSH7DF"};
#define OIL_COMMANDS 3

static void send(elm_t *e, const char *command, uint32_t timeout_ms, uint32_t now_ms) {
    snprintf(e->command, sizeof e->command, "%s", command);
    char line[sizeof e->command + 1];
    snprintf(line, sizeof line, "%s\r", command);
    e->rx_len = 0;
    e->waiting = true;
    e->sent_ms = now_ms;
    e->timeout_ms = timeout_ms;
    e->cb.write(e->cb.ctx, line);
}

static void fail(elm_t *e, const char *step, const char *error) {
    e->phase = ELM_FAILED;
    e->waiting = false;
    e->cb.failed(e->cb.ctx, step, error);
}

static void log_answer(elm_t *e, const char *command, char *lines[], int n) {
    char out[160];
    int len = snprintf(out, sizeof out, "elm327: %s ->", command);
    for (int i = 0; i < n && len < (int)sizeof out; i++) {
        len += snprintf(out + len, sizeof out - (size_t)len, "%s%s", i ? " | " : " ", lines[i]);
    }
    e->cb.log(e->cb.ctx, out);
}

static void start_scan(elm_t *e, uint32_t now_ms);
static void start_listen(elm_t *e, uint32_t now_ms);

static void send_next_poll(elm_t *e, uint32_t now_ms) {
    // A scan or a listen waits for the oil's header switching to be over.
    if (e->scan_requested && e->oil_step == 0) {
        start_scan(e, now_ms);
        return;
    }
    if (e->listen_requested && e->oil_step == 0) {
        start_listen(e, now_ms);
        return;
    }
    if (e->oil_step > 0 || (e->oil_misses < ELM_OIL_MAX_MISSES && now_ms - e->oil_ms >= ELM_OIL_INTERVAL_MS)) {
        if (e->oil_step == 0) {
            e->oil_ms = now_ms;
        }
        send(e, oil_commands[e->oil_step], ELM_TIMEOUT_MS, now_ms);
        e->oil_step = (e->oil_step + 1) % OIL_COMMANDS;
        return;
    }
    if (now_ms - e->battery_ms >= ELM_BATTERY_INTERVAL_MS) {
        e->battery_ms = now_ms;
        send(e, "ATRV", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    char command[12];
    if (e->multi_pid) {
        snprintf(command, sizeof command, "%s%s", MULTI_POLL, e->single_response ? "1" : "");
    } else {
        snprintf(command, sizeof command, "01%02X%s", poll_pids[e->step], e->single_response ? "1" : "");
        e->step = (e->step + 1) % POLL_COUNT;
    }
    send(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void start_probe(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_PROBE;
    e->step = 0;
    send(e, probe_commands[0], ELM_TIMEOUT_MS, now_ms);
}

static void start_poll(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_POLL;
    e->step = 0;
    e->battery_ms = now_ms - ELM_BATTERY_INTERVAL_MS;
    e->oil_ms = now_ms;
    e->ecu_ms = now_ms;
    e->cb.ready(e->cb.ctx);
    send_next_poll(e, now_ms);
}

static void handle_init(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (e->step > 0) {
        bool ok = false;
        for (int i = 0; i < n; i++) {
            ok = ok || strcmp(lines[i], "OK") == 0;
        }
        if (!ok) {
            char error[96];
            snprintf(error, sizeof error, "%s answered \"%s\"", e->command, n ? lines[0] : "");
            fail(e, "ELM327 init", error);
            return;
        }
    }
    if (++e->step < INIT_COUNT) {
        send(e, init_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    e->cb.log(e->cb.ctx, "elm327: ready");
    start_probe(e, now_ms);
}

// The RAM scan (asked from the settings page) looks for where the engine
// ECU keeps the ethanol %, to
// read it there without tapping the flex-fuel sensor's wire. This ECU
// ignores Subaru SSM reads (A0) and locks its ROM behind security access
// (UDS 23 answers 7F 23 33), but reads its RAM, from 0xFFF80000, with a
// UDS ReadMemoryByAddress: 23 14, 4 address bytes, 1 size byte (checked
// 2026-11-14). A float that is a whole number of ECU ADC steps once
// turned back into the sensor's volts (25%/V - 12.5%, the tune's scaling)
// is logged as a candidate, as is one that is such a voltage itself.
static void send_scan_read(elm_t *e, uint32_t now_ms) {
    char command[sizeof e->command];
    snprintf(command, sizeof command, "2314%08lX%02X", (unsigned long)e->scan_addr, ELM_SCAN_CHUNK);
    send(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void scan_word(elm_t *e, uint32_t addr, const uint8_t *b) {
    uint32_t bits = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    float f;
    memcpy(&f, &bits, sizeof f);
    const char *what = NULL;
    if (f >= 1 && f <= 100 && sensors_on_ecu_adc_grid((f + 12.5f) / 25)) {
        what = "ethanol %";
    } else if (f >= ETHANOL_SENSOR_MIN_VOLTS && f <= ETHANOL_SENSOR_MAX_VOLTS && sensors_on_ecu_adc_grid(f)) {
        what = "sensor volts";
    }
    if (what && e->scan_candidates++ < ELM_SCAN_LOGGED) {
        char line[64];
        snprintf(line, sizeof line, "scan: %08lX = %.3f (%s?)", (unsigned long)addr, (double)f, what);
        e->cb.log(e->cb.ctx, line);
    }
}

static void start_scan(elm_t *e, uint32_t now_ms) {
    e->scan_requested = false;
    e->phase = ELM_SCAN;
    e->scan_addr = ELM_SCAN_START;
    e->scan_candidates = 0;
    e->cb.log(e->cb.ctx, "scan: started");
    state_set_status(e->state, "DEEP SCAN\nreaching the ECU");
    send(e, "ATSH7E0", ELM_TIMEOUT_MS, now_ms);
}

static void finish_scan(elm_t *e, uint32_t now_ms) {
    char line[80];
    snprintf(line, sizeof line, "scan: stopped at %08lX, %d candidates", (unsigned long)e->scan_addr,
             e->scan_candidates);
    e->cb.log(e->cb.ctx, line);
    send(e, "ATSH7DF", ELM_TIMEOUT_MS, now_ms);
}

static void handle_scan(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (strcmp(e->command, "ATSH7E0") == 0) {
        send_scan_read(e, now_ms);
        return;
    }
    if (strcmp(e->command, "ATSH7DF") == 0) {
        start_poll(e, now_ms);
        return;
    }
    isotp_t answer = {0};
    bool whole = false;
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        if (elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_feed(&answer, &f)) {
            whole = true;
        }
    }
    if (!whole || answer.len != 1 + ELM_SCAN_CHUNK || answer.data[0] != 0x63) {
        log_answer(e, e->command, lines, n);
        finish_scan(e, now_ms);
        return;
    }
    for (int i = 0; i < ELM_SCAN_CHUNK; i += 4) {
        scan_word(e, e->scan_addr + (uint32_t)i, answer.data + 1 + i);
    }
    e->scan_addr += ELM_SCAN_CHUNK;
    uint32_t done = e->scan_addr - ELM_SCAN_START;
    if (done >= ELM_SCAN_MAX) {
        finish_scan(e, now_ms);
        return;
    }
    char status[64];
    snprintf(status, sizeof status, "DEEP SCAN\n%lu KB of ECU RAM\n%d candidates", (unsigned long)(done / 1024),
             e->scan_candidates);
    state_set_status(e->state, status);
    send_scan_read(e, now_ms);
}

// Listening sets the dongle to print raw frames (ATCAF0) through an ID
// mask of 0x700 (ATCM700), then moves the window (ATCF) and monitors
// (ATMA) in turn. Any character stops a monitor; the dongle stops by
// itself, with BUFFER FULL, when BLE can't keep up. At the end, the
// session starts over (ATZ) rather than undo all that.
static void start_listen(elm_t *e, uint32_t now_ms) {
    e->listen_requested = false;
    e->phase = ELM_LISTEN;
    e->listen_ms = now_ms;
    e->window = 0;
    memset(&e->sniff, 0, sizeof e->sniff);
    e->cb.log(e->cb.ctx, "sniff: listening, hands off");
    state_set_status(e->state, "LISTEN\nhands off the car");
    send(e, "ATCAF0", ELM_TIMEOUT_MS, now_ms);
}

static void send_window(elm_t *e, uint32_t now_ms) {
    char command[16];
    snprintf(command, sizeof command, "ATCF%03X", e->window * 0x100);
    send(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void handle_listen(elm_t *e, uint32_t now_ms) {
    if (strcmp(e->command, "ATCAF0") == 0) {
        send(e, "ATCM700", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    if (strncmp(e->command, "ATCF", 4) == 0 || strcmp(e->command, "ATCM700") == 0) {
        if (strcmp(e->command, "ATCM700") == 0) {
            send_window(e, now_ms);
            return;
        }
        e->monitoring = true;
        e->stop_sent = false;
        e->monitor_ms = now_ms;
        send(e, "ATMA", ELM_LISTEN_DWELL_MS + ELM_TIMEOUT_MS, now_ms);
        return;
    }
    // A monitor ended: next window, or the next stage.
    e->monitoring = false;
    e->window = (e->window + 1) % ELM_LISTEN_WINDOWS;
    uint32_t elapsed = now_ms - e->listen_ms;
    if (elapsed >= ELM_LISTEN_MS) {
        sniff_report(&e->sniff, e->cb.log, e->cb.ctx);
        state_set_status(e->state, "SYNC\nlistening done, see the log");
        elm_start(e, e->state, e->cb, now_ms);
        return;
    }
    char status[96];
    if (elapsed < ELM_LISTEN_BASELINE_MS) {
        snprintf(status, sizeof status, "LISTEN\nhands off the car\n%lus", (unsigned long)((ELM_LISTEN_BASELINE_MS - elapsed) / 1000));
    } else {
        if (!e->sniff.baseline_over) {
            sniff_end_baseline(&e->sniff);
            e->cb.log(e->cb.ctx, "sniff: baseline over, presses");
        }
        snprintf(status, sizeof status, "LISTEN\npress SPORT on and off,\nagain and again\n%lus", (unsigned long)((ELM_LISTEN_MS - elapsed) / 1000));
    }
    state_set_status(e->state, status);
    send_window(e, now_ms);
}

static void next_probe(elm_t *e, uint32_t now_ms) {
    if (++e->step < PROBE_COUNT) {
        send(e, probe_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    start_poll(e, now_ms);
}

// check_multi_poll turns multi_pid on if lines, the answer to MULTI_POLL,
// hold every PID it asked for.
static void check_multi_poll(elm_t *e, char *lines[], int n) {
    state_t scratch = {0};
    isotp_t answer = {0};
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        if (elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_feed(&answer, &f)) {
            obd_decode_message(answer.data, answer.len, &scratch, 0);
        }
    }
    e->multi_pid = scratch.readings[SIG_RPM].valid && scratch.readings[SIG_COOLANT].valid &&
                   scratch.readings[SIG_AFR].valid && scratch.readings[SIG_IAT].valid;
    e->cb.log(e->cb.ctx, e->multi_pid ? "elm327: polling every PID at once"
                                      : "elm327: no multi-PID answer, polling one PID at a time");
}

static void handle_poll(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    bool obd = strncmp(e->command, "01", 2) == 0 || strcmp(e->command, "2101") == 0;
    // Mode 01 requests are even-length; a count digit makes them odd.
    size_t len = strlen(e->command);
    bool counted = strncmp(e->command, "01", 2) == 0 && len % 2 == 1;
    if (counted && n == 1 && strcmp(lines[0], "?") == 0) {
        // Older or cloned firmware doesn't know the count digit.
        e->single_response = false;
        e->cb.log(e->cb.ctx, "elm327: no response count digit, polling slower");
        char command[sizeof e->command];
        snprintf(command, sizeof command, "%.*s", (int)len - 1, e->command);
        send(e, command, ELM_TIMEOUT_MS, now_ms);
        return;
    }
    isotp_t answer = {0};
    signal_t got = SIG_COUNT;
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        float volts;
        if (obd && elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_feed(&answer, &f)) {
            got = obd_decode_message(answer.data, answer.len, e->state, now_ms);
        } else if (strcmp(e->command, "ATRV") == 0 && elm_parse_voltage(lines[i], &volts)) {
            state_set(e->state, SIG_BATTERY, volts, now_ms);
        }
    }
    if (got != SIG_COUNT) {
        e->ecu_ms = now_ms;
        e->ecu_answered = true;
    }
    // An unanswered 2101 only counts against it while the ECU answers
    // the rest: with the car off, nothing answers, and it must still be
    // asked once the car starts.
    if (strcmp(e->command, "2101") == 0 && (got == SIG_OIL || e->ecu_answered)) {
        e->oil_misses = got == SIG_OIL ? 0 : e->oil_misses + 1;
        if (e->oil_misses == ELM_OIL_MAX_MISSES) {
            log_answer(e, "2101 (oil temperature, giving up)", lines, n);
        }
    }
    send_next_poll(e, now_ms);
}

static void handle_response(elm_t *e, uint32_t now_ms) {
    e->rx[e->rx_len] = 0;
    char *lines[ELM_MAX_LINES];
    int n = elm_split_lines(e->rx, e->command, lines, ELM_MAX_LINES);
    e->attempt = 0;
    switch (e->phase) {
    case ELM_INIT:
        handle_init(e, lines, n, now_ms);
        break;
    case ELM_PROBE:
        log_answer(e, e->command, lines, n);
        if (strcmp(e->command, MULTI_POLL "1") == 0) {
            check_multi_poll(e, lines, n);
        }
        next_probe(e, now_ms);
        break;
    case ELM_SCAN:
        handle_scan(e, lines, n, now_ms);
        break;
    case ELM_LISTEN:
        handle_listen(e, now_ms);
        break;
    case ELM_POLL:
        handle_poll(e, lines, n, now_ms);
        break;
    default:
        break;
    }
}

void elm_start(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms) {
    // Not *e = (elm_t){...}: with the sniff table, that temporary would
    // take 5KB of the Pico's 2KB stack.
    memset(e, 0, sizeof *e);
    e->cb = cb;
    e->state = state;
    e->phase = ELM_INIT;
    e->single_response = true;
    send(e, init_commands[0], ELM_RESET_TIMEOUT_MS, now_ms);
}

void elm_receive(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms) {
    for (size_t i = 0; i < len && e->waiting; i++) {
        // A monitor prints frames until stopped: each line is fed as it
        // comes, the buffer would never hold them all.
        if (e->monitoring && (data[i] == '\r' || data[i] == '\n')) {
            e->rx[e->rx_len] = 0;
            can_frame_t f;
            if (elm_parse_frame(e->rx, &f)) {
                sniff_feed(&e->sniff, &f);
            }
            e->rx_len = 0;
        } else if (data[i] == '>') {
            e->waiting = false;
            handle_response(e, now_ms);
        } else if (data[i] != 0 && e->rx_len < sizeof e->rx - 1) {
            e->rx[e->rx_len++] = (char)data[i];
        }
    }
}

void elm_tick(elm_t *e, uint32_t now_ms) {
    if (e->monitoring && !e->stop_sent && now_ms - e->monitor_ms >= ELM_LISTEN_DWELL_MS) {
        e->stop_sent = true;
        e->cb.write(e->cb.ctx, "\r");
        return;
    }
    if (!e->waiting || now_ms - e->sent_ms <= e->timeout_ms) {
        return;
    }
    if (e->attempt < ELM_RETRIES) {
        e->attempt++;
        char command[sizeof e->command];
        memcpy(command, e->command, sizeof command);
        send(e, command, e->timeout_ms, now_ms);
        return;
    }
    e->attempt = 0;
    char error[64];
    snprintf(error, sizeof error, "no answer to %s", e->command);
    if (e->phase == ELM_PROBE) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        next_probe(e, now_ms);
        return;
    }
    if (e->phase == ELM_LISTEN) {
        e->cb.log(e->cb.ctx, error);
        sniff_report(&e->sniff, e->cb.log, e->cb.ctx);
        elm_start(e, e->state, e->cb, now_ms);
        return;
    }
    if (e->phase == ELM_SCAN) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        if (strcmp(e->command, "ATSH7DF") == 0) {
            start_poll(e, now_ms);
        } else {
            finish_scan(e, now_ms);
        }
        return;
    }
    fail(e, e->phase == ELM_POLL ? "OBD poll" : "ELM327 init", error);
}

void elm_scan_ram(elm_t *e) {
    if (e->phase == ELM_POLL) {
        e->scan_requested = true;
    }
}

void elm_listen_can(elm_t *e) {
    if (e->phase == ELM_POLL) {
        e->listen_requested = true;
    }
}

int elm_split_lines(char *raw, const char *command, char *lines[], int max) {
    int n = 0;
    for (char *line = strtok(raw, "\r\n"); line && n < max; line = strtok(NULL, "\r\n")) {
        while (*line == ' ') {
            line++;
        }
        size_t len = strlen(line);
        while (len > 0 && line[len - 1] == ' ') {
            line[--len] = 0;
        }
        if (len == 0 || strcmp(line, command) == 0 || strncmp(line, "SEARCHING", 9) == 0) {
            continue;
        }
        lines[n++] = line;
    }
    return n;
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

bool elm_parse_frame(const char *line, can_frame_t *f) {
    int digits[3 + 16];
    int n = 0;
    for (; *line; line++) {
        if (*line == ' ') {
            continue;
        }
        int d = hex_digit(*line);
        if (d < 0 || n == (int)(sizeof digits / sizeof digits[0])) {
            return false;
        }
        digits[n++] = d;
    }
    int data_digits = n - 3;
    if (data_digits < 2 || data_digits % 2 != 0) {
        return false;
    }
    *f = (can_frame_t){.id = (uint32_t)(digits[0] << 8 | digits[1] << 4 | digits[2]), .dlc = (uint8_t)(data_digits / 2)};
    for (int i = 0; i < f->dlc; i++) {
        f->data[i] = (uint8_t)(digits[3 + 2 * i] << 4 | digits[4 + 2 * i]);
    }
    return true;
}

bool elm_parse_voltage(const char *line, float *volts) {
    char *end;
    float v = strtof(line, &end);
    if (end == line || (*end != 'V' && *end != 0)) {
        return false;
    }
    *volts = v;
    return true;
}
