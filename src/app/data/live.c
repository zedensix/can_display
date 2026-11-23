#include "live.h"

#include <limits.h>
#include <math.h>

// The reading if it's fresh, else NAN.
static float fresh(const state_t *s, signal_t sig, uint32_t now_ms, uint32_t max_age_ms) {
    const reading_t *r = &s->readings[sig];
    return reading_is_stale(r, now_ms, max_age_ms) ? NAN : r->value;
}

static void put16(uint8_t *out, int v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
}

// v scaled and rounded, clamped to lo..hi, or none when v is NAN.
static int scaled(float v, float scale, int lo, int hi, int none) {
    if (isnan(v)) {
        return none;
    }
    float x = roundf(v * scale);
    return x<(float)lo ? lo : x>(float) hi ? hi : (int)x;
}

void live_encode(const state_t *s, uint32_t now_ms, uint8_t out[LIVE_SIZE]) {
    out[0] = LIVE_VERSION;
    put16(out + 1, scaled(fresh(s, SIG_OIL, now_ms, SLOW_STALE_MS), 10, INT16_MIN + 1, INT16_MAX, INT16_MIN));
    put16(out + 3, scaled(fresh(s, SIG_COOLANT, now_ms, SLOW_STALE_MS), 10, INT16_MIN + 1, INT16_MAX, INT16_MIN));
    out[5] = (uint8_t)scaled(fresh(s, SIG_IAT, now_ms, FAST_STALE_MS), 1, INT8_MIN + 1, INT8_MAX, INT8_MIN);
    out[6] = (uint8_t)scaled(fresh(s, SIG_AFR, now_ms, FAST_STALE_MS), 10, 1, 255, 0);
    out[7] = (uint8_t)scaled(fresh(s, SIG_BATTERY, now_ms, FAST_STALE_MS), 10, 1, 255, 0);
    out[8] = (uint8_t)scaled(fresh(s, SIG_ETHANOL, now_ms, SLOW_STALE_MS), 1, 0, 100, 0xFF);
    put16(out + 9, scaled(fresh(s, SIG_RPM, now_ms, FAST_STALE_MS), 1, 0, 0xFFFE, 0xFFFF));
    out[11] = (uint8_t)state_gear(s, now_ms, FAST_STALE_MS);
    out[12] = (uint8_t)scaled(fresh(s, SIG_AMBIENT, now_ms, SLOW_STALE_MS), 1, INT8_MIN + 1, INT8_MAX, INT8_MIN);
    float odometer = fresh(s, SIG_ODOMETER, now_ms, SLOW_STALE_MS);
    int km = scaled(isnan(odometer) ? s->trip_km : odometer, 10, 0, 0xFFFFFF, 0);
    out[13] = (uint8_t)km;
    out[14] = (uint8_t)(km >> 8);
    out[15] = (uint8_t)(km >> 16);
    out[16] = (uint8_t)((unsigned)state_drive_mode(s, now_ms) |
                        (state_off_throttle(s, now_ms, FAST_STALE_MS) ? LIVE_OFF_THROTTLE : 0) |
                        (isnan(odometer) ? LIVE_TRIP : 0));
}
