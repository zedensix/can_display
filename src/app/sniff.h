#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "obd.h"

// sniff_t finds the CAN ID behind a button (the GT86's sport mode, for
// one) by elimination. During a baseline, nobody touches anything: every
// ID whose data changes is noise (counters, RPM, temperatures). Then the
// driver presses the button over and over: an ID that changes now, and
// not before, is a candidate.

#define SNIFF_MAX_IDS 160
// sniff_report logs at most this many candidates.
#define SNIFF_REPORTED 24

typedef struct {
    uint16_t id;
    uint8_t dlc, changes;
    bool noisy;
    // data is the latest frame; before is the data when the baseline
    // ended, after the first different data seen since.
    uint8_t data[8], before[8], after[8];
    uint8_t before_dlc, after_dlc;
} sniff_id_t;

typedef struct {
    sniff_id_t ids[SNIFF_MAX_IDS];
    int count;
    bool baseline_over;
} sniff_t;

// sniff_feed takes a frame seen on the bus.
void sniff_feed(sniff_t *s, const can_frame_t *f);

// sniff_end_baseline starts the presses.
void sniff_end_baseline(sniff_t *s);

// sniff_report logs a summary, then each candidate as
// "sniff: <ID> x<changes>: <data before> -> <data after>".
void sniff_report(const sniff_t *s, void (*log)(void *ctx, const char *line), void *ctx);
