#include <stdio.h>
#include <string.h>

#include "elm_internal.h"

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

void send_next_poll_command(elm_t *e, uint32_t now_ms) {
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

void start_polling(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_POLL;
    e->step = 0;
    e->battery_ms = now_ms - ELM_BATTERY_INTERVAL_MS;
    e->engine_ms = now_ms;
    e->ecu_ms = now_ms;
    e->cb.ready(e->cb.ctx);
    send_next_poll_command(e, now_ms);
}

void enable_multi_pid_if_answered(elm_t *e, char *lines[], int n) {
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

void handle_poll_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
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
