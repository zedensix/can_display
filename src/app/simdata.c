#include "simdata.h"

#include <math.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

// noise is a uniform random value in [-amplitude, amplitude], from a
// fixed-seed generator so runs repeat.
static float noise(float amplitude) {
    static uint32_t seed = 1;
    seed = seed * 1664525u + 1013904223u;
    return ((float)(seed >> 8) / (float)(1u << 24) * 2 - 1) * amplitude;
}

void simdata_tick(state_t *state, uint32_t now_ms) {
    float t = (float)now_ms / 1000;
    float warmup = clampf(t / 90, 0, 1);
    state_set(state, SIG_COOLANT, 20 + warmup * 70 + noise(1.5f), now_ms);
    state_set(state, SIG_OIL, 20 + warmup * 95 + noise(2), now_ms);
    state_set(state, SIG_RPM, 850 + clampf(sinf(t / 6), 0, 1) * 6700 + noise(50), now_ms);
    state_set(state, SIG_BATTERY, 14.2f + sinf(t / 5) * 0.3f + noise(0.05f), now_ms);
    state_set(state, SIG_AFR, 14.7f + sinf(t / 2) * 1.5f + noise(0.2f), now_ms);
    state_set(state, SIG_ETHANOL, 10 + noise(1), now_ms);
}
