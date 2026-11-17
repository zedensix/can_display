#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "obd.h"

// sniff_t finds the CAN bits behind things the driver can set by hand,
// by comparing the bus in each state, one step at a time: the driver
// sets a state (sniff_steps' prompt), clicks Next, and the bus is
// listened to. Within a step nobody touches anything: every bit that
// changes then is noise (counters, RPM, temperatures). A candidate is a
// bit, quiet within every step, that differs between two states and
// agrees between the steps of one state: the steps back to the base
// state weed out what drifts (a temperature). Bit by bit, not ID by ID:
// a flag often shares its frame with a rolling counter.
//
// This run looks for the doors, the boot, the cruise control stalk (held
// up, down, pulled) and its main switch. An earlier run, through the
// stability control's modes, found 0x0D3 (obd.h).

// The bus showed 36 IDs through the dongle: room for more.
#define SNIFF_MAX_IDS 64
// At most 16: sniff_id_t.seen holds a bit per step.
#define SNIFF_STEPS 15
// sniff_log_candidates logs at most this many candidates.
#define SNIFF_REPORTED 24

// The states: everything shut and the cruise control off (the base), a
// door or the boot open, the stalk held one way, the cruise control on.
typedef enum {
    SNIFF_SHUT,
    SNIFF_DRIVER,
    SNIFF_PASSENGER,
    SNIFF_BOOT,
    SNIFF_STALK_UP,
    SNIFF_STALK_DOWN,
    SNIFF_STALK_PULLED,
    SNIFF_CRUISE_ON,
    SNIFF_STATES,
} sniff_state_t;

typedef struct {
    const char *prompt; // what the driver does before Next, for the status screen
    sniff_state_t state;
} sniff_step_t;

// sniff_steps: each state in turn, back to the base between them.
extern const sniff_step_t sniff_steps[SNIFF_STEPS];

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint16_t seen;                // a bit per step the ID was seen in
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

// sniff_log_candidates logs a summary and a legend, then each candidate
// as "sniff: <ID> bits <mask>: S <data> D <data> ...", the mask over its
// bytes, the data from each state's first step, a letter per state (S
// shut, D driver's door, P passenger's, B boot, U stalk up, W down, C
// pulled, M cruise on): a log line holds 200 characters.
void sniff_log_candidates(const sniff_t *s, void (*log)(void *ctx, const char *line), void *ctx);
