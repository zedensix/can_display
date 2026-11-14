#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "obd.h"
#include "signals.h"
#include "sniff.h"

// An ELM327 session, transport-agnostic: commands go out through
// callbacks.write, the dongle's output comes back through elm_receive.
// It never blocks: elm_tick handles timeouts, so it runs from the same
// loop as the BLE stack.
//
// The session resets and configures the dongle (echo off, CAN headers
// on, ISO 15765-4 11bit 500kbps), logs its raw answers to a few probes,
// then polls RPM, AFR, coolant and intake temperatures (all in one request if the
// ECU answers that), plus the battery voltage about once a second (ATRV,
// answered by the dongle itself) and the oil temperature every few
// seconds. With headers on, each reply line is a CAN frame; multi-frame
// answers are reassembled (isotp_feed).

#define ELM_TIMEOUT_MS 2000
// ATZ reboots the dongle's firmware: it gets longer.
#define ELM_RESET_TIMEOUT_MS 5000
// A command that times out is resent this many times before the
// session fails: over BLE a notification gets lost now and then.
#define ELM_RETRIES 2
#define ELM_BATTERY_INTERVAL_MS 1000
#define ELM_OIL_INTERVAL_MS 3000
#define ELM_OIL_MAX_MISSES 3

// The RAM scan reads ELM_SCAN_CHUNK bytes at a time (whole floats, and
// the answer fits isotp_t), from ELM_SCAN_START until the ECU refuses an
// address or ELM_SCAN_MAX bytes, and logs at most ELM_SCAN_LOGGED
// candidates.
#define ELM_SCAN_START 0xFFF80000u
#define ELM_SCAN_MAX (256u * 1024)
#define ELM_SCAN_CHUNK 60
#define ELM_SCAN_LOGGED 40

// Listening to the CAN bus (elm_listen_can) for sniff_t: the bus is far
// busier than BLE can carry, so the dongle listens to one window of 256
// IDs at a time (ATCF/ATCM), ELM_LISTEN_DWELL_MS each, round and round:
// ELM_LISTEN_BASELINE_MS hands off, then presses until ELM_LISTEN_MS.
#define ELM_LISTEN_WINDOWS 8
#define ELM_LISTEN_DWELL_MS 1500
#define ELM_LISTEN_BASELINE_MS 45000
#define ELM_LISTEN_MS 105000

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
    ELM_SCAN,
    ELM_LISTEN,
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
    // multi_pid polls every PID in one request, if the ECU answers that.
    bool multi_pid;
    // scan_requested starts a RAM scan at the next poll (elm_scan_ram);
    // scan_addr is the next address it reads.
    bool scan_requested;
    uint32_t scan_addr;
    int scan_candidates;
    // listen_requested starts listening to the bus at the next poll
    // (elm_listen_can); monitoring is while the dongle prints frames.
    bool listen_requested, monitoring, stop_sent;
    uint32_t listen_ms, monitor_ms;
    int window;
    sniff_t sniff;
} elm_t;

void elm_start(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms);
void elm_receive(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms);
void elm_tick(elm_t *e, uint32_t now_ms);

// elm_scan_ram asks for a scan of the engine ECU's RAM for the ethanol %
// (see elm327.c): it starts at the next poll, shows its progress as the
// status, logs its candidates, and polling resumes after it.
void elm_scan_ram(elm_t *e);

// elm_listen_can asks for a sniff of the CAN bus (see sniff_t and
// ELM_LISTEN_*): it starts at the next poll, tells the driver what to do
// as the status, logs the candidates, then restarts the session.
void elm_listen_can(elm_t *e);

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
