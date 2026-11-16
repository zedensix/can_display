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
// over BLE instead of five), with the response count digit like 010C1.
// This dongle (IOS-Vlink) counts frames, not answers: a "1" got it to
// stop after the first of the answer's frames (log, 2026-11-16). The 15
// bytes take 3 frames.
#define MULTI_POLL "010C24050F11"
#define MULTI_POLL_FRAMES "3"
static const char *const probe_commands[] = {
    "ATI", "ATRV", "0100", "0120", "0140", "010C", MULTI_POLL MULTI_POLL_FRAMES};
#define PROBE_COUNT (int)(sizeof probe_commands / sizeof probe_commands[0])

// RPM comes every other request: the shift light follows it. The
// throttle says whether a lean AFR is a deceleration.
static const uint8_t poll_pids[] = {PID_RPM, PID_LAMBDA, PID_RPM, PID_THROTTLE, PID_RPM, PID_COOLANT, PID_RPM, PID_IAT};
#define POLL_COUNT (int)(sizeof poll_pids / sizeof poll_pids[0])

// The oil temperature and the ethanol % (see obd.h) are asked to the
// engine ECU directly, which takes switching the header there and back
// (ATSH7E0, the reads, ATSH7DF). They change slowly: every few seconds
// is plenty. After a few answers without its value while the ECU answers
// the rest, the session stops asking for one.
static const struct {
    const char *command;
    signal_t signal;
} engine_reads[ELM_ENGINE_READS] = {{"2101", SIG_OIL}, {ETHANOL_RAM_READ, SIG_ETHANOL}};

static bool engine_read_wanted(const elm_t *e, int read) {
    return e->engine_misses[read] < ELM_ENGINE_MAX_MISSES;
}

// send_next_engine_command sends the engine ECU sequence's next command:
// engine_step 0 is ATSH7E0, 1 to ELM_ENGINE_READS the reads still wanted,
// then ATSH7DF, after which engine_step is back to 0.
static void send_next_engine_command(elm_t *e, uint32_t now_ms);

static void send_command(elm_t *e, const char *command, uint32_t timeout_ms, uint32_t now_ms) {
    snprintf(e->command, sizeof e->command, "%s", command);
    char line[sizeof e->command + 1];
    snprintf(line, sizeof line, "%s\r", command);
    e->rx_len = 0;
    e->waiting = true;
    e->sent_ms = now_ms;
    e->timeout_ms = timeout_ms;
    e->cb.write(e->cb.ctx, line);
}

static void fail_session(elm_t *e, const char *step, const char *error) {
    e->phase = ELM_FAILED;
    e->waiting = false;
    e->cb.failed(e->cb.ctx, step, error);
}

static void log_command_answer(elm_t *e, const char *command, char *lines[], int n) {
    char out[160];
    int len = snprintf(out, sizeof out, "elm327: %s ->", command);
    for (int i = 0; i < n && len < (int)sizeof out; i++) {
        len += snprintf(out + len, sizeof out - (size_t)len, "%s%s", i ? " | " : " ", lines[i]);
    }
    e->cb.log(e->cb.ctx, out);
}

static void start_ram_scan(elm_t *e, uint32_t now_ms);
static void start_can_listen(elm_t *e, uint32_t now_ms);

static void send_next_poll_command(elm_t *e, uint32_t now_ms) {
    // A scan or a listen waits for the engine ECU's header switching to be over.
    if (e->scan_requested && e->engine_step == 0) {
        start_ram_scan(e, now_ms);
        return;
    }
    if (e->listen_requested && e->engine_step == 0) {
        start_can_listen(e, now_ms);
        return;
    }
    bool any_wanted = false;
    for (int i = 0; i < ELM_ENGINE_READS; i++) {
        any_wanted = any_wanted || engine_read_wanted(e, i);
    }
    if (e->engine_step > 0 || (any_wanted && now_ms - e->engine_ms >= ELM_ENGINE_INTERVAL_MS)) {
        if (e->engine_step == 0) {
            e->engine_ms = now_ms;
        }
        send_next_engine_command(e, now_ms);
        return;
    }
    if (now_ms - e->battery_ms >= ELM_BATTERY_INTERVAL_MS) {
        e->battery_ms = now_ms;
        send_command(e, "ATRV", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    char command[sizeof e->command];
    if (e->multi_pid) {
        snprintf(command, sizeof command, "%s%s", MULTI_POLL, e->single_response ? MULTI_POLL_FRAMES : "");
    } else {
        snprintf(command, sizeof command, "01%02X%s", poll_pids[e->step], e->single_response ? "1" : "");
        e->step = (e->step + 1) % POLL_COUNT;
    }
    send_command(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void send_next_engine_command(elm_t *e, uint32_t now_ms) {
    if (e->engine_step == 0) {
        send_command(e, "ATSH7E0", ELM_TIMEOUT_MS, now_ms);
        e->engine_step = 1;
        return;
    }
    while (e->engine_step <= ELM_ENGINE_READS && !engine_read_wanted(e, e->engine_step - 1)) {
        e->engine_step++;
    }
    if (e->engine_step <= ELM_ENGINE_READS) {
        send_command(e, engine_reads[e->engine_step - 1].command, ELM_TIMEOUT_MS, now_ms);
        e->engine_step++;
        return;
    }
    send_command(e, "ATSH7DF", ELM_TIMEOUT_MS, now_ms);
    e->engine_step = 0;
}

static void start_probes(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_PROBE;
    e->step = 0;
    send_command(e, probe_commands[0], ELM_TIMEOUT_MS, now_ms);
}

static void start_polling(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_POLL;
    e->step = 0;
    e->battery_ms = now_ms - ELM_BATTERY_INTERVAL_MS;
    e->engine_ms = now_ms;
    e->ecu_ms = now_ms;
    e->cb.ready(e->cb.ctx);
    send_next_poll_command(e, now_ms);
}

static void handle_init_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (e->step > 0) {
        bool ok = false;
        for (int i = 0; i < n; i++) {
            ok = ok || strcmp(lines[i], "OK") == 0;
        }
        if (!ok) {
            char error[96];
            snprintf(error, sizeof error, "%s answered \"%s\"", e->command, n ? lines[0] : "");
            fail_session(e, "ELM327 init", error);
            return;
        }
    }
    if (++e->step < INIT_COUNT) {
        send_command(e, init_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    e->cb.log(e->cb.ctx, "elm327: ready");
    start_probes(e, now_ms);
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
static void send_ram_scan_read(elm_t *e, uint32_t now_ms) {
    char command[sizeof e->command];
    snprintf(command, sizeof command, "2314%08lX%02X", (unsigned long)e->scan_addr, ELM_SCAN_CHUNK);
    send_command(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void check_ram_word_for_ethanol(elm_t *e, uint32_t addr, const uint8_t *b) {
    uint32_t bits = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    float f;
    memcpy(&f, &bits, sizeof f);
    const char *what = NULL;
    if (f >= 1 && f <= 100 && sensors_is_whole_ecu_adc_step((f + 12.5f) / 25)) {
        what = "ethanol %";
    } else if (f >= ETHANOL_SENSOR_MIN_VOLTS && f <= ETHANOL_SENSOR_MAX_VOLTS && sensors_is_whole_ecu_adc_step(f)) {
        what = "sensor volts";
    }
    if (what && e->scan_candidates++ < ELM_SCAN_LOGGED) {
        char line[64];
        snprintf(line, sizeof line, "scan: %08lX = %.3f (%s?)", (unsigned long)addr, (double)f, what);
        e->cb.log(e->cb.ctx, line);
    }
}

static void start_ram_scan(elm_t *e, uint32_t now_ms) {
    e->scan_requested = false;
    e->phase = ELM_SCAN;
    e->scan_addr = ELM_SCAN_START;
    e->scan_candidates = 0;
    e->cb.log(e->cb.ctx, "scan: started");
    state_set_status(e->state, "DEEP SCAN\nreaching the ECU");
    send_command(e, "ATSH7E0", ELM_TIMEOUT_MS, now_ms);
}

static void finish_ram_scan(elm_t *e, uint32_t now_ms) {
    char line[80];
    snprintf(line, sizeof line, "scan: stopped at %08lX, %d candidates", (unsigned long)e->scan_addr,
             e->scan_candidates);
    e->cb.log(e->cb.ctx, line);
    send_command(e, "ATSH7DF", ELM_TIMEOUT_MS, now_ms);
}

static void handle_ram_scan_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (strcmp(e->command, "ATSH7E0") == 0) {
        send_ram_scan_read(e, now_ms);
        return;
    }
    if (strcmp(e->command, "ATSH7DF") == 0) {
        start_polling(e, now_ms);
        return;
    }
    isotp_t answer = {0};
    bool whole = false;
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        if (elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_add_frame(&answer, &f)) {
            whole = true;
        }
    }
    if (!whole || answer.len != 1 + ELM_SCAN_CHUNK || answer.data[0] != 0x63) {
        log_command_answer(e, e->command, lines, n);
        finish_ram_scan(e, now_ms);
        return;
    }
    for (int i = 0; i < ELM_SCAN_CHUNK; i += 4) {
        check_ram_word_for_ethanol(e, e->scan_addr + (uint32_t)i, answer.data + 1 + i);
    }
    e->scan_addr += ELM_SCAN_CHUNK;
    uint32_t done = e->scan_addr - ELM_SCAN_START;
    if (done >= ELM_SCAN_MAX) {
        finish_ram_scan(e, now_ms);
        return;
    }
    char status[64];
    snprintf(status, sizeof status, "DEEP SCAN\n%lu KB of ECU RAM\n%d candidates", (unsigned long)(done / 1024),
             e->scan_candidates);
    state_set_status(e->state, status);
    send_ram_scan_read(e, now_ms);
}

// Listening sets the dongle to print raw frames (ATCAF0) through an ID
// mask of 0x700 (ATCM700), then, for each of sniff_steps, prompts the
// driver and waits for a click, then moves the window (ATCF) and monitors
// (ATMA) in turn, ELM_LISTEN_ROUNDS times over the bus. Any character
// stops a monitor; the dongle stops by itself, with BUFFER FULL, when BLE
// can't keep up. At the end, the session starts over (ATZ) rather than
// undo all that.
static void start_can_listen(elm_t *e, uint32_t now_ms) {
    e->listen_requested = false;
    e->phase = ELM_LISTEN;
    memset(&e->sniff, 0, sizeof e->sniff);
    e->cb.log(e->cb.ctx, "sniff: listening");
    state_set_status(e->state, "LISTEN\nsetting the dongle up");
    send_command(e, "ATCAF0", ELM_TIMEOUT_MS, now_ms);
}

static void prompt_listen_step(elm_t *e) {
    e->listen_awaiting_click = true;
    char status[128];
    snprintf(status, sizeof status, "LISTEN %d/%d\n%s\nthen click", e->sniff.step + 1, SNIFF_STEPS,
             sniff_steps[e->sniff.step].prompt);
    state_set_status(e->state, status);
}

static void send_listen_window_filter(elm_t *e, uint32_t now_ms) {
    char command[16];
    snprintf(command, sizeof command, "ATCF%03X", e->windows % ELM_LISTEN_WINDOWS * 0x100);
    send_command(e, command, ELM_TIMEOUT_MS, now_ms);
    char status[64];
    snprintf(status, sizeof status, "LISTEN %d/%d\nhands off\n%d%%", e->sniff.step + 1, SNIFF_STEPS,
             100 * e->windows / (ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS));
    state_set_status(e->state, status);
}

bool elm_listen_click(elm_t *e, uint32_t now_ms) {
    if (e->phase != ELM_LISTEN || !e->listen_awaiting_click) {
        return false;
    }
    e->listen_awaiting_click = false;
    e->windows = 0;
    char line[48];
    snprintf(line, sizeof line, "sniff: step %d/%d", e->sniff.step + 1, SNIFF_STEPS);
    e->cb.log(e->cb.ctx, line);
    send_listen_window_filter(e, now_ms);
    return true;
}

static void handle_can_listen_answer(elm_t *e, uint32_t now_ms) {
    if (strcmp(e->command, "ATCAF0") == 0) {
        send_command(e, "ATCM700", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    if (strcmp(e->command, "ATCM700") == 0) {
        prompt_listen_step(e);
        return;
    }
    if (strncmp(e->command, "ATCF", 4) == 0) {
        e->monitoring = true;
        e->stop_sent = false;
        e->monitor_ms = now_ms;
        send_command(e, "ATMA", ELM_LISTEN_DWELL_MS + ELM_TIMEOUT_MS, now_ms);
        return;
    }
    // A monitor ended: next window, the next step, or done.
    e->monitoring = false;
    if (++e->windows < ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS) {
        send_listen_window_filter(e, now_ms);
        return;
    }
    if (++e->sniff.step < SNIFF_STEPS) {
        prompt_listen_step(e);
        return;
    }
    sniff_log_candidates(&e->sniff, e->cb.log, e->cb.ctx);
    state_set_status(e->state, "SYNC\nlistening done, see the log");
    elm_start_session(e, e->state, e->cb, now_ms);
}

static void send_next_probe(elm_t *e, uint32_t now_ms) {
    if (++e->step < PROBE_COUNT) {
        send_command(e, probe_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    start_polling(e, now_ms);
}

// enable_multi_pid_if_answered turns multi_pid on if lines, the answer to MULTI_POLL,
// hold every PID it asked for.
static void enable_multi_pid_if_answered(elm_t *e, char *lines[], int n) {
    state_t scratch = {0};
    isotp_t answer = {0};
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        if (elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_add_frame(&answer, &f)) {
            obd_decode_message(answer.data, answer.len, &scratch, 0);
        }
    }
    e->multi_pid = scratch.readings[SIG_RPM].valid && scratch.readings[SIG_COOLANT].valid &&
                   scratch.readings[SIG_AFR].valid && scratch.readings[SIG_IAT].valid &&
                   scratch.readings[SIG_THROTTLE].valid;
    e->cb.log(e->cb.ctx, e->multi_pid ? "elm327: polling every PID at once"
                                      : "elm327: no multi-PID answer, polling one PID at a time");
}

static void handle_poll_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    bool obd = strncmp(e->command, "01", 2) == 0 || strcmp(e->command, "2101") == 0 ||
               strcmp(e->command, ETHANOL_RAM_READ) == 0;
    // Mode 01 requests are even-length; a count digit makes them odd.
    size_t len = strlen(e->command);
    bool counted = strncmp(e->command, "01", 2) == 0 && len % 2 == 1;
    if (counted && n == 1 && strcmp(lines[0], "?") == 0) {
        // Older or cloned firmware doesn't know the count digit.
        e->single_response = false;
        e->cb.log(e->cb.ctx, "elm327: no response count digit, polling slower");
        char command[sizeof e->command];
        snprintf(command, sizeof command, "%.*s", (int)len - 1, e->command);
        send_command(e, command, ELM_TIMEOUT_MS, now_ms);
        return;
    }
    isotp_t answer = {0};
    signal_t got = SIG_COUNT;
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        float volts;
        if (obd && elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_add_frame(&answer, &f)) {
            got = obd_decode_message(answer.data, answer.len, e->state, now_ms);
        } else if (strcmp(e->command, "ATRV") == 0 && elm_parse_voltage(lines[i], &volts)) {
            state_set_reading(e->state, SIG_BATTERY, volts, now_ms);
        }
    }
    if (got != SIG_COUNT) {
        e->ecu_ms = now_ms;
        e->ecu_answered = true;
    }
    // An engine ECU read left unanswered only counts against it while the
    // ECU answers the rest: with the car off, nothing answers, and it
    // must still be asked once the car starts.
    for (int i = 0; i < ELM_ENGINE_READS; i++) {
        if (strcmp(e->command, engine_reads[i].command) == 0 && (got == engine_reads[i].signal || e->ecu_answered)) {
            e->engine_misses[i] = got == engine_reads[i].signal ? 0 : e->engine_misses[i] + 1;
            if (e->engine_misses[i] == ELM_ENGINE_MAX_MISSES) {
                char what[48];
                snprintf(what, sizeof what, "%s (giving up)", engine_reads[i].command);
                log_command_answer(e, what, lines, n);
            }
        }
    }
    send_next_poll_command(e, now_ms);
}

static void handle_dongle_answer(elm_t *e, uint32_t now_ms) {
    e->rx[e->rx_len] = 0;
    char *lines[ELM_MAX_LINES];
    int n = elm_split_lines(e->rx, e->command, lines, ELM_MAX_LINES);
    e->attempt = 0;
    switch (e->phase) {
    case ELM_INIT:
        handle_init_answer(e, lines, n, now_ms);
        break;
    case ELM_PROBE:
        log_command_answer(e, e->command, lines, n);
        if (strcmp(e->command, MULTI_POLL MULTI_POLL_FRAMES) == 0) {
            enable_multi_pid_if_answered(e, lines, n);
        }
        send_next_probe(e, now_ms);
        break;
    case ELM_SCAN:
        handle_ram_scan_answer(e, lines, n, now_ms);
        break;
    case ELM_LISTEN:
        handle_can_listen_answer(e, now_ms);
        break;
    case ELM_POLL:
        handle_poll_answer(e, lines, n, now_ms);
        break;
    default:
        break;
    }
}

void elm_start_session(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms) {
    // Not *e = (elm_t){...}: with the sniff table, that temporary would
    // take 5KB of the Pico's 2KB stack.
    memset(e, 0, sizeof *e);
    e->cb = cb;
    e->state = state;
    e->phase = ELM_INIT;
    e->single_response = true;
    send_command(e, init_commands[0], ELM_RESET_TIMEOUT_MS, now_ms);
}

void elm_feed_dongle_output(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms) {
    for (size_t i = 0; i < len && e->waiting; i++) {
        // A monitor prints frames until stopped: each line is fed as it
        // comes, the buffer would never hold them all.
        if (e->monitoring && (data[i] == '\r' || data[i] == '\n')) {
            e->rx[e->rx_len] = 0;
            can_frame_t f;
            if (elm_parse_frame(e->rx, &f)) {
                sniff_add_frame(&e->sniff, &f);
            }
            e->rx_len = 0;
        } else if (data[i] == '>') {
            e->waiting = false;
            handle_dongle_answer(e, now_ms);
        } else if (data[i] != 0 && e->rx_len < sizeof e->rx - 1) {
            e->rx[e->rx_len++] = (char)data[i];
        }
    }
}

void elm_check_timers(elm_t *e, uint32_t now_ms) {
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
        send_command(e, command, e->timeout_ms, now_ms);
        return;
    }
    e->attempt = 0;
    char error[64];
    snprintf(error, sizeof error, "no answer to %s", e->command);
    if (e->phase == ELM_PROBE) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        send_next_probe(e, now_ms);
        return;
    }
    if (e->phase == ELM_LISTEN) {
        e->cb.log(e->cb.ctx, error);
        sniff_log_candidates(&e->sniff, e->cb.log, e->cb.ctx);
        elm_start_session(e, e->state, e->cb, now_ms);
        return;
    }
    if (e->phase == ELM_SCAN) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        if (strcmp(e->command, "ATSH7DF") == 0) {
            start_polling(e, now_ms);
        } else {
            finish_ram_scan(e, now_ms);
        }
        return;
    }
    fail_session(e, e->phase == ELM_POLL ? "OBD poll" : "ELM327 init", error);
}

void elm_request_ram_scan(elm_t *e) {
    if (e->phase == ELM_POLL) {
        e->scan_requested = true;
    }
}

void elm_request_can_listen(elm_t *e) {
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

static int hex_digit_value(char c) {
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
        int d = hex_digit_value(*line);
        if (d < 0 || n == (int)(sizeof digits / sizeof digits[0])) {
            return false;
        }
        digits[n++] = d;
    }
    int data_digits = n - 3;
    if (data_digits < 2 || data_digits % 2 != 0) {
        return false;
    }
    *f =
        (can_frame_t){.id = (uint32_t)(digits[0] << 8 | digits[1] << 4 | digits[2]), .dlc = (uint8_t)(data_digits / 2)};
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
