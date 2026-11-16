#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "obd.h"

// sniff_t finds the CAN bits behind the VSC / TRC OFF button by
// comparing the bus in each of its modes, one step at a time: the driver
// sets a mode (sniff_steps' prompt), clicks, and the bus is listened to.
// Within a step nobody touches anything: every bit that changes then is
// noise (counters, RPM, temperatures). A candidate is a bit, quiet within
// every step, that differs between two modes and agrees between the
// steps of one mode: the steps back to "on" weed out what drifts (a
// temperature). Bit by bit, not ID by ID: a flag often shares its frame
// with a rolling counter (a first run, ID by ID, found 15 of 36 IDs noisy
// and no candidate).

#define SNIFF_MAX_IDS 160
#define SNIFF_STEPS 5
// sniff_log_candidates logs at most this many candidates.
#define SNIFF_REPORTED 24

typedef enum { SNIFF_ALL_ON, SNIFF_TRC_OFF, SNIFF_ALL_OFF, SNIFF_MODES } sniff_mode_t;

typedef struct {
    const char *prompt; // what the driver does before clicking, for the status screen
    sniff_mode_t mode;
} sniff_step_t;

// sniff_steps: everything on, TRC OFF (a short press), back on, everything
// off (TRC OFF held a few seconds), back on.
extern const sniff_step_t sniff_steps[SNIFF_STEPS];

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t seen;                 // a bit per step the ID was seen in
    uint8_t noise[8];             // the bits that changed within a step
    uint8_t data[SNIFF_STEPS][8]; // the latest frame in each step, zero past dlc
} sniff_id_t;

typedef struct {
    sniff_id_t ids[SNIFF_MAX_IDS];
    int count;
    int step;
} sniff_t;

// sniff_add_frame takes a frame seen on the bus during step s->step.
void sniff_add_frame(sniff_t *s, const can_frame_t *f);

// sniff_log_candidates logs a summary, then each candidate as
// "sniff: <ID> bits <mask>: on <data> trc <data> off <data>", the mask
// over its bytes, the data from each mode's first step.
void sniff_log_candidates(const sniff_t *s, void (*log)(void *ctx, const char *line), void *ctx);
