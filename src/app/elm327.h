#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "obd.h"
#include "signals.h"

// An ELM327 session, transport-agnostic: commands go out through
// callbacks.write, the dongle's output comes back through elm_receive.
// It never blocks: elm_tick handles timeouts, so it runs from the same
// loop as the BLE stack.
//
// The session resets and configures the dongle (echo off, CAN headers
// on, ISO 15765-4 11bit 500kbps), logs its raw answers to a few probes,
// then polls RPM, AFR and coolant temperature, plus the battery voltage
// about once a second (ATRV, answered by the dongle itself) and the oil
// temperature every few seconds. With headers on, each reply line is a
// CAN frame; multi-frame answers are reassembled (isotp_feed).

#define ELM_TIMEOUT_MS 2000
// ATZ reboots the dongle's firmware: it gets longer.
#define ELM_RESET_TIMEOUT_MS 5000
// A command that times out is resent this many times before the
// session fails: over BLE a notification gets lost now and then.
#define ELM_RETRIES 2
#define ELM_BATTERY_INTERVAL_MS 1000
#define ELM_OIL_INTERVAL_MS 3000
#define ELM_OIL_MAX_MISSES 3

typedef struct {
    // write sends one command, "\r" included.
    void (*write)(void *ctx, const char *command);
    void (*log)(void *ctx, const char *line);
    // ready is called once, when the dongle answers and polling starts.
    void (*ready)(void *ctx);
    // failed is called once, when the session gives up.
    void (*failed)(void *ctx, const char *step, const char *error);
    void *ctx;
} elm_callbacks_t;

typedef enum {
    ELM_OFF,
    ELM_INIT,
    ELM_PROBE,
    ELM_POLL,
    ELM_FAILED,
} elm_phase_t;

typedef struct {
    elm_callbacks_t cb;
    state_t *state;
    elm_phase_t phase;
    int step;
    char command[16];
    bool waiting;
    uint32_t sent_ms, timeout_ms;
    int attempt;
    char rx[512];
    size_t rx_len;
    // single_response appends the ELM's "expect 1 reply" digit to OBD
    // requests ("010C1"), so it answers as soon as the engine ECU does
    // instead of waiting out its timeout for other ECUs. Turned off if
    // the dongle rejects it.
    bool single_response;
    uint32_t battery_ms;
    uint32_t oil_ms;
    int oil_step, oil_misses;
    // ecu_ms is when the engine ECU last answered (or polling started):
    // a long silence means the car is off. ecu_answered is whether it
    // answered at all this session.
    uint32_t ecu_ms;
    bool ecu_answered;
} elm_t;

void elm_start(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms);
void elm_receive(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms);
void elm_tick(elm_t *e, uint32_t now_ms);

#define ELM_MAX_LINES 16

// elm_split_lines cuts raw (everything before the ">" prompt) into
// trimmed, non-empty lines, in place, dropping the echo of command and
// the "SEARCHING..." line. It returns how many it stored in lines.
int elm_split_lines(char *raw, const char *command, char *lines[], int max);

// elm_parse_frame parses a reply line printed with ATH1 for an 11bit
// frame: 3 hex digits of ID, then up to 8 data bytes, spaces optional
// ("7E8 03 41 05 7B" or "7E80341057B"). Anything else (NO DATA, CAN
// ERROR, 29bit IDs) gives false.
bool elm_parse_frame(const char *line, can_frame_t *f);

// elm_parse_voltage parses an ATRV answer like "12.6V".
bool elm_parse_voltage(const char *line, float *volts);
