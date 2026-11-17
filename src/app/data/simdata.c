#include "simdata.h"

// A tour of the dash's scenarios, one scene after the other, then over
// again: each holds long enough to read, and nothing jitters (no noise:
// on the panel, digits changing every frame flicker). RPM goes from rpm0
// to rpm1 over the scene, in gear (0: N, coasting at 50 km/h, or
// stopped while idling); the temperatures and the battery ease over the
// first 2 seconds from the scene before; mode is set as the scene starts
// (the simulator's m changes it after).
typedef struct {
    uint32_t ms;
    float rpm0, rpm1;
    int gear;
    float throttle, afr, oil, coolant, intake, battery;
    drive_mode_t mode;
    uint8_t doors;
} scene_t;

static const scene_t scenes[] = {
    // Cold start, idling: oil and coolant blue.
    {8000, 950, 950, 0, 15, 14.7f, 35, 30, 18, 14.4f, DRIVE_NORMAL, 0},
    // Warming up, gently in 3rd.
    {8000, 2000, 2600, 3, 25, 14.7f, 70, 72, 22, 14.3f, DRIVE_NORMAL, 0},
    // Warm, cruising in 6th.
    {10000, 2400, 2450, 6, 22, 14.6f, 95, 90, 26, 14.2f, DRIVE_NORMAL, 0},
    // A pull in 2nd, the shift light filling up, then 3rd to the limiter:
    // the whole screen flashes for its last half second.
    {4000, 3500, 7000, 2, 81, 12.3f, 98, 91, 27, 14.1f, DRIVE_NORMAL, 0},
    {4500, 4600, 7450, 3, 81, 12.2f, 100, 92, 28, 14.1f, DRIVE_NORMAL, 0},
    // Lifting off: the fuel cut reads lean, white, no alarm.
    {4000, 7000, 2500, 3, 14, 20.4f, 101, 92, 28, 14.2f, DRIVE_NORMAL, 0},
    // On track: TRC off, VSC sport, then everything off, getting hot.
    {8000, 4000, 6500, 3, 60, 12.6f, 112, 96, 40, 14.1f, DRIVE_TRC_OFF, 0},
    {8000, 4500, 6800, 3, 70, 12.4f, 116, 98, 45, 14.1f, DRIVE_SPORT, 0},
    // Everything off, the oil red, the intake heat-soaked: they blink.
    {8000, 4200, 6600, 3, 70, 12.4f, 124, 100, 63, 14.0f, DRIVE_VSC_OFF, 0},
    // The oil at 134: the whole screen.
    {6000, 3000, 3000, 4, 30, 14.7f, 134, 102, 55, 14.1f, DRIVE_NORMAL, 0},
    // Cooling down in 5th.
    {10000, 2200, 2200, 5, 20, 14.7f, 104, 92, 32, 14.2f, DRIVE_NORMAL, 0},
    // Lean under load: the AFR blinks red.
    {6000, 3500, 4000, 4, 55, 17.2f, 100, 90, 30, 14.2f, DRIVE_NORMAL, 0},
    // Idling with the battery low: it blinks.
    {6000, 850, 850, 0, 15, 14.7f, 98, 90, 30, 11.4f, DRIVE_NORMAL, 0},
    // Parked, the driver's door open, then the boot too: the door screen.
    {4000, 850, 850, 0, 15, 14.7f, 96, 89, 30, 14.2f, DRIVE_NORMAL, DOOR_DRIVER},
    {4000, 850, 850, 0, 15, 14.7f, 95, 89, 30, 14.2f, DRIVE_NORMAL, DOOR_DRIVER | DOOR_BOOT},
};
#define SCENES (int)(sizeof scenes / sizeof scenes[0])
#define EASE_MS 2000

static float lerp(float a, float b, float t) {
    return a + (b - a) * (t < 0 ? 0 : t > 1 ? 1 : t);
}

void simdata_update(state_t *state, uint32_t now_ms) {
    static int last_scene = -1;
    uint32_t tour = 0;
    for (int i = 0; i < SCENES; i++) {
        tour += scenes[i].ms;
    }
    uint32_t at = now_ms % tour;
    int k = 0;
    while (at >= scenes[k].ms) {
        at -= scenes[k++].ms;
    }
    const scene_t *s = &scenes[k], *before = &scenes[(k + SCENES - 1) % SCENES];
    float t = (float)at / (float)s->ms, ease = (float)at / EASE_MS;
    if (k != last_scene) {
        last_scene = k;
        state_set_drive_mode(state, s->mode, now_ms);
    }

    float rpm = lerp(s->rpm0, s->rpm1, t);
    bool lifting = s->afr > 19;
    state_set_reading(state, SIG_RPM, rpm, now_ms);
    float speed = s->gear ? rpm / (GEAR_RPM_PER_KMH * gear_ratios[s->gear - 1]) : rpm > 1000 ? 50 : 0;
    state_set_reading(state, SIG_SPEED, speed, now_ms);
    state_set_reading(state, SIG_THROTTLE, s->throttle, now_ms);
    state_set_reading(state, SIG_PEDAL, lifting ? 0 : (s->throttle - 14) * 100 / 67, now_ms);
    state_set_reading(state, SIG_LOAD, lifting ? 12 : s->throttle * 1.1f, now_ms);
    state_set_reading(state, SIG_TIMING, lifting ? 30 : 34 - s->throttle * 0.2f, now_ms);
    state_set_reading(state, SIG_AFR, s->afr, now_ms);
    state_set_reading(state, SIG_AFR_TARGET, s->afr > 15 ? 14.7f : s->afr, now_ms);
    state_set_reading(state, SIG_STFT, 1.5f, now_ms);
    state_set_reading(state, SIG_LTFT, 2.3f, now_ms);
    state_set_reading(state, SIG_RAIL, 5 + s->throttle * 0.18f, now_ms);
    state_set_reading(state, SIG_OIL, lerp(before->oil, s->oil, ease), now_ms);
    state_set_reading(state, SIG_COOLANT, lerp(before->coolant, s->coolant, ease), now_ms);
    state_set_reading(state, SIG_IAT, lerp(before->intake, s->intake, ease), now_ms);
    state_set_reading(state, SIG_BATTERY, lerp(before->battery, s->battery, ease), now_ms);
    state_set_reading(state, SIG_AMBIENT, 14, now_ms);
    state_set_reading(state, SIG_ETHANOL, 67, now_ms);
    state_set_doors(state, s->doors, now_ms);
}
