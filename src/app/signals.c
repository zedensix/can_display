#include "signals.h"

#include <stdio.h>

void state_set(state_t *s, signal_t sig, float value, uint32_t now_ms) {
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
}

void state_set_status(state_t *s, const char *status) {
    snprintf(s->status, sizeof s->status, "%s", status);
}

bool reading_stale(const reading_t *r, uint32_t now_ms, uint32_t max_age_ms) {
    return !r->valid || now_ms - r->updated_ms > max_age_ms;
}
