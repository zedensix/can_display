#include "signals.h"

#include <stdio.h>

void state_set_reading(state_t *s, signal_t sig, float value, uint32_t now_ms) {
    reading_t *r = &s->readings[sig];
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

void state_set_drive_mode(state_t *s, drive_mode_t mode, uint32_t now_ms) {
    s->drive_mode = mode;
    s->drive_mode_ms = now_ms;
    s->drive_mode_read = true;
}

drive_mode_t state_drive_mode(const state_t *s, uint32_t now_ms) {
    return s->drive_mode_read && now_ms - s->drive_mode_ms <= DRIVE_MODE_STALE_MS ? s->drive_mode : DRIVE_NORMAL;
}
