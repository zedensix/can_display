#include "simdata.h"

#include <math.h>

static float clamp_float(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

// random_noise is a uniform random value in [-amplitude, amplitude], from a
// fixed-seed generator so runs repeat.
static float random_noise(float amplitude) {
    static uint32_t seed = 1;
    seed = seed * 1664525u + 1013904223u;
    return ((float)(seed >> 8) / (float)(1u << 24) * 2 - 1) * amplitude;
}

void simdata_update(state_t *state, uint32_t now_ms) {
    float t = (float)now_ms / 1000;
    float warmup = clamp_float(t / 90, 0, 1);
    state_set_reading(state, SIG_COOLANT, 20 + warmup * 70 + random_noise(1.5f), now_ms);
    state_set_reading(state, SIG_OIL, 20 + warmup * 95 + random_noise(2), now_ms);
    state_set_reading(state, SIG_IAT, 24 + warmup * 12 + random_noise(1), now_ms);
    float pull = clamp_float(sinf(t / 6), 0, 1);
    // Revving up on the throttle, then off it while the RPM falls: the
    // fuel is cut, the AFR reads lean.
    bool lifting = cosf(t / 6) < 0 || pull == 0;
    state_set_reading(state, SIG_RPM, 850 + pull * 6700 + random_noise(50), now_ms);
    state_set_reading(state, SIG_THROTTLE, lifting ? 14 : 14 + pull * 80, now_ms);
    state_set_reading(state, SIG_BATTERY, 14.2f + sinf(t / 5) * 0.3f + random_noise(0.05f), now_ms);
    state_set_reading(state, SIG_AFR, lifting && pull > 0 ? 20.4f : 14.7f + sinf(t / 2) * 1.5f + random_noise(0.2f),
                      now_ms);
    state_set_reading(state, SIG_ETHANOL, 10 + random_noise(1), now_ms);
}
