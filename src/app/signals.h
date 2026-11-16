#pragma once

#include <stdbool.h>
#include <stdint.h>

// signal_t names every value the dashboard shows.
typedef enum {
    SIG_RPM,
    SIG_COOLANT,
    SIG_IAT,
    SIG_OIL,
    SIG_BATTERY,
    SIG_AFR,
    SIG_ETHANOL,
    SIG_THROTTLE, // %, absolute: not shown, it tells the AFR alarm a deceleration
    SIG_COUNT,
} signal_t;

// reading_t is one signal's latest value, and the lowest and highest
// since boot (the session's extremes). Times are milliseconds since boot.
typedef struct {
    float value;
    bool valid;
    uint32_t updated_ms;
    float min, max;
} reading_t;

// state_t is everything the UI draws. status is the data source's own
// report while it isn't delivering (scan, connect, failure): while it
// isn't empty, the gauges page shows it instead of the tiles.
typedef struct {
    reading_t readings[SIG_COUNT];
    uint32_t throttle_closed_ms; // the last throttle reading that counted as closed
    char status[192];
} state_t;

void state_set_reading(state_t *s, signal_t sig, float value, uint32_t now_ms);
void state_set_status(state_t *s, const char *status);

// reading_is_stale is true for a reading never set, or older than max_age_ms:
// the UI shows "--" rather than a frozen or zero value.
bool reading_is_stale(const reading_t *r, uint32_t now_ms, uint32_t max_age_ms);

// THROTTLE_CLOSED_MARGIN is how far above the lowest throttle seen since
// boot still counts as closed: the ECU's closed position isn't 0%, and
// rather than a guessed figure, the session's minimum (idling, lifting
// off) is the closed throttle.
#define THROTTLE_CLOSED_MARGIN 5

// THROTTLE_REOPEN_HOLD_MS is how long a throttle still counts as closed
// once opened again: the AFR reading lags the fuel coming back.
#define THROTTLE_REOPEN_HOLD_MS 1000

// state_off_throttle is true while a fresh throttle reading says the foot
// is off, or was THROTTLE_REOPEN_HOLD_MS ago: decelerating, the ECU cuts
// the fuel and the AFR reads lean (20.4 on this car), or idling. Without
// a throttle reading, false.
bool state_off_throttle(const state_t *s, uint32_t now_ms, uint32_t max_age_ms);
