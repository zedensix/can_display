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
    char status[192];
} state_t;

void state_set(state_t *s, signal_t sig, float value, uint32_t now_ms);
void state_set_status(state_t *s, const char *status);

// reading_stale is true for a reading never set, or older than max_age_ms:
// the UI shows "--" rather than a frozen or zero value.
bool reading_stale(const reading_t *r, uint32_t now_ms, uint32_t max_age_ms);
