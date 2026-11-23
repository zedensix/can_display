#include "signals.h"

#include <stdio.h>

void state_set_reading(state_t *s, signal_t sig, float value, uint32_t now_ms) {
    reading_t *r = &s->readings[sig];
    if (sig == SIG_SPEED && r->valid && now_ms - r->updated_ms <= TRIP_MAX_GAP_MS) {
        // The mean of the two readings, over the time between them.
        s->trip_km += (r->value + value) / 2 * (float)(now_ms - r->updated_ms) / 3600000;
    }
    if (!r->valid || value < r->min) {
        r->min = value;
    }
    if (!r->valid || value > r->max) {
        r->max = value;
    }
    r->value = value;
    r->valid = true;
    r->updated_ms = now_ms;
    if (sig == SIG_THROTTLE && value <= r->min + THROTTLE_CLOSED_MARGIN) {
        s->throttle_closed_ms = now_ms;
    }
}

void state_set_status(state_t *s, const char *status) {
    snprintf(s->status, sizeof s->status, "%s", status);
}

bool reading_is_stale(const reading_t *r, uint32_t now_ms, uint32_t max_age_ms) {
    return !r->valid || now_ms - r->updated_ms > max_age_ms;
}

bool state_off_throttle(const state_t *s, uint32_t now_ms, uint32_t max_age_ms) {
    return !reading_is_stale(&s->readings[SIG_THROTTLE], now_ms, max_age_ms) &&
           now_ms - s->throttle_closed_ms <= THROTTLE_REOPEN_HOLD_MS;
}

const float gear_ratios[GEARS] = {3.626f, 2.188f, 1.541f, 1.213f, 1.000f, 0.767f};

int state_gear(const state_t *s, uint32_t now_ms, uint32_t max_age_ms) {
    const reading_t *rpm = &s->readings[SIG_RPM], *speed = &s->readings[SIG_SPEED];
    if (reading_is_stale(rpm, now_ms, max_age_ms) || reading_is_stale(speed, now_ms, max_age_ms) ||
        speed->value < GEAR_MIN_KMH) {
        return 0;
    }
    float per_kmh = rpm->value / speed->value / GEAR_RPM_PER_KMH;
    for (int g = 0; g < GEARS; g++) {
        if (per_kmh > gear_ratios[g] * (1 - GEAR_TOLERANCE) && per_kmh < gear_ratios[g] * (1 + GEAR_TOLERANCE)) {
            return g + 1;
        }
    }
    return 0;
}

void state_set_drive_mode(state_t *s, drive_mode_t mode, uint32_t now_ms) {
    s->drive_mode = mode;
    s->drive_mode_ms = now_ms;
    s->drive_mode_read = true;
}

drive_mode_t state_drive_mode(const state_t *s, uint32_t now_ms) {
    bool running = s->readings[SIG_RPM].valid && s->readings[SIG_RPM].value >= DRIVE_MODE_MIN_RPM;
    return running && s->drive_mode_read && now_ms - s->drive_mode_ms <= DRIVE_MODE_STALE_MS ? s->drive_mode
                                                                                             : DRIVE_COMFORT;
}

void state_set_doors(state_t *s, uint8_t doors, uint32_t now_ms) {
    s->doors = doors;
    s->doors_ms = now_ms;
    s->doors_read = true;
}

uint8_t state_doors(const state_t *s, uint32_t now_ms) {
    return s->doors_read && now_ms - s->doors_ms <= DOORS_STALE_MS ? s->doors : 0;
}
