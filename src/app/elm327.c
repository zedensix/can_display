#include "elm327.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
// PIDs the engine ECU supports. A0 is a Subaru SSM memory read, asked to
// the engine ECU: 8 bytes at 0x11CC38, the tune's "Ethanol Sensor
// Scaling" (25.0 and -12.5 as floats if SSM works on this ECU), the
// first step towards reading the ethanol % from its RAM.
static const char *const probe_commands[] = {"ATI",     "ATRV",         "0100",   "0120", "0140",
                                             "010C",    "ATSH7E0",      "A00011CC3807", "ATSH7DF"};
#define PROBE_COUNT (int)(sizeof probe_commands / sizeof probe_commands[0])

// RPM comes twice per cycle: it's the only value that changes faster
// than a human can read.
static const uint8_t poll_pids[] = {PID_RPM, PID_LAMBDA, PID_RPM, PID_COOLANT};
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

static void send_next_poll(elm_t *e, uint32_t now_ms) {
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
    char command[8];
    snprintf(command, sizeof command, "01%02X%s", poll_pids[e->step], e->single_response ? "1" : "");
    e->step = (e->step + 1) % POLL_COUNT;
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

static void next_probe(elm_t *e, uint32_t now_ms) {
    if (++e->step < PROBE_COUNT) {
        send(e, probe_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    start_poll(e, now_ms);
}

static void handle_poll(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    bool obd = strncmp(e->command, "01", 2) == 0 || strcmp(e->command, "2101") == 0;
    bool counted = strncmp(e->command, "01", 2) == 0 && strlen(e->command) == 5;
    if (counted && n == 1 && strcmp(lines[0], "?") == 0) {
        // Older or cloned firmware doesn't know the count digit.
        e->single_response = false;
        e->cb.log(e->cb.ctx, "elm327: no response count digit, polling slower");
        char command[8];
        snprintf(command, sizeof command, "%.4s", e->command);
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
        next_probe(e, now_ms);
        break;
    case ELM_POLL:
        handle_poll(e, lines, n, now_ms);
        break;
    default:
        break;
    }
}

void elm_start(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms) {
    *e = (elm_t){.cb = cb, .state = state, .phase = ELM_INIT, .single_response = true};
    send(e, init_commands[0], ELM_RESET_TIMEOUT_MS, now_ms);
}

void elm_receive(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms) {
    for (size_t i = 0; i < len && e->waiting; i++) {
        if (data[i] == '>') {
            e->waiting = false;
            handle_response(e, now_ms);
        } else if (data[i] != 0 && e->rx_len < sizeof e->rx - 1) {
            e->rx[e->rx_len++] = (char)data[i];
        }
    }
}

void elm_tick(elm_t *e, uint32_t now_ms) {
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
    fail(e, e->phase == ELM_POLL ? "OBD poll" : "ELM327 init", error);
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
