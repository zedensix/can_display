#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nmea.h"

// A GPS lap timer. The start/finish line is a point and the heading cars
// cross it with: a lap ends when two consecutive fixes fall on either
// side of the line, moving forward, within LAP_GATE_HALF_WIDTH_M of the
// point. The crossing time is interpolated between the two fixes on the
// GPS clock: at 10 fixes a second, about 10ms of error instead of 100.

#define LAP_GATE_HALF_WIDTH_M 15
// A crossing closer than this to the previous one is GPS noise, or
// driving back and forth over the line in the pits.
#define LAP_MIN_MS 10000
// Below this speed, a GPS course means nothing.
#define LAP_MOVING_KMH 15

// lap_line_t is the start/finish line. heading is the course crossing
// it, in degrees clockwise from north, -1 until known: a line set
// standing still takes the course of the first fix moving (lap_line_settle).
typedef struct {
    bool set;
    int32_t lat, lon;
    int16_t heading;
} lap_line_t;

typedef struct {
    gps_fix_t prev;
    // timing is once a lap has started: start_gps_ms is its crossing on
    // the GPS clock, start_at_ms the same moment on the Pico's.
    bool timing;
    uint32_t start_gps_ms, start_at_ms;
    int laps;
    uint32_t last_ms, best_ms;
    // completed_at_ms is when the last lap ended, on the Pico's clock.
    uint32_t completed_at_ms;
} laptimer_t;

// lap_line_at is a line through fix, its heading fix's course if moving.
lap_line_t lap_line_at(const gps_fix_t *fix);

// lap_line_settle gives line the course of fix if it has none yet and
// fix is moving. It returns whether it changed line.
bool lap_line_settle(lap_line_t *line, const gps_fix_t *fix);

// laptimer_feed takes each new fix, received at now_ms, and returns true
// when it ended a lap (last_ms, best_ms and laps are then updated).
bool laptimer_feed(laptimer_t *l, const lap_line_t *line, const gps_fix_t *fix, uint32_t now_ms);

// laptimer_running_ms is the current lap's time so far.
uint32_t laptimer_running_ms(const laptimer_t *l, uint32_t now_ms);
