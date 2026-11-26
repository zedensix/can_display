// The gauges render_ui draws, each from a fixed state, as the panel would
// show them; render_ui_screens.c draws the other screens.

#include <stdio.h>
#include <string.h>

#include "render_ui.h"

const uint32_t now = 100000;

// state_while_driving is a session so far (the extremes) and its latest readings.
state_t state_while_driving(void) {
    state_t s = {0};
    const float low[SIG_COUNT] = {[SIG_RPM] = 820,       [SIG_COOLANT] = 84, [SIG_IAT] = 22,     [SIG_OIL] = 88,
                                  [SIG_BATTERY] = 13.9f, [SIG_AFR] = 11.8f,  [SIG_ETHANOL] = 67, [SIG_THROTTLE] = 14,
                                  [SIG_PEDAL] = 0,       [SIG_LOAD] = 12,    [SIG_SPEED] = 0,    [SIG_AMBIENT] = 14};
    const float high[SIG_COUNT] = {[SIG_RPM] = 7350,      [SIG_COOLANT] = 96, [SIG_IAT] = 41,     [SIG_OIL] = 112,
                                   [SIG_BATTERY] = 14.4f, [SIG_AFR] = 15.9f,  [SIG_ETHANOL] = 69, [SIG_THROTTLE] = 92,
                                   [SIG_PEDAL] = 100,     [SIG_LOAD] = 91,    [SIG_SPEED] = 168,  [SIG_AMBIENT] = 14};
    const float v[SIG_COUNT] = {[SIG_RPM] = 6400,      [SIG_COOLANT] = 91, [SIG_IAT] = 31,     [SIG_OIL] = 104,
                                [SIG_BATTERY] = 14.2f, [SIG_AFR] = 14.7f,  [SIG_ETHANOL] = 68, [SIG_THROTTLE] = 45,
                                [SIG_PEDAL] = 48,      [SIG_LOAD] = 63,    [SIG_SPEED] = 119,  [SIG_AMBIENT] = 14};
    for (int i = 0; i < SIG_COUNT; i++) {
        state_set_reading(&s, (signal_t)i, low[i], now - 60000);
        state_set_reading(&s, (signal_t)i, high[i], now - 60000);
        state_set_reading(&s, (signal_t)i, v[i], now);
    }
    // 3rd gear at 6400 rpm and 119 km/h; 37.4 km since boot, no odometer.
    s.trip_km = 37.4f;
    s.readings[SIG_ODOMETER].valid = false;
    return s;
}

settings_t cfg = {.brightness = BRIGHTNESS_MAX, .shift_rpm = 7000};

void render_screen(canvas_t *c, const state_t *s, screen_t screen) {
    ui_render(c, s, &screen, &cfg, now);
}

static void shot_grid(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){0});
}

// The oil too hot, the AFR lean, the intake heat-soaked, the ethanol
// stale; near the shift point, every LED pair but the last lit.
static state_t state_with_alarms(void) {
    state_t s = state_while_driving();
    s.readings[SIG_RPM].value = 6900;
    s.readings[SIG_OIL].value = 124;
    s.readings[SIG_AFR].value = 17.2f;
    s.readings[SIG_IAT].value = 63;
    s.readings[SIG_ETHANOL].updated_ms = 0;
    return s;
}

// On track, the traction control off: the track grid.
static void shot_track(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_OIL].value = 116;
    s.readings[SIG_AFR].value = 12.4f;
    state_set_drive_mode(&s, DRIVE_DRIFT, now);
    render_screen(c, &s, (screen_t){0});
}

static void shot_alarms(canvas_t *c) {
    state_t s = state_with_alarms();
    render_screen(c, &s, (screen_t){0});
}

// The same alarms between two blinks.
static void shot_alarms_unlit(canvas_t *c) {
    state_t s = state_with_alarms();
    screen_t screen = {0};
    ui_render(c, &s, &screen, &cfg, now + 250);
}

// Past the shift light's RPM: every LED flashes blue.
static void shot_shift(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_RPM].value = 7150;
    render_screen(c, &s, (screen_t){0});
}

// A cold start: oil and coolant not warm yet.
static void shot_cold(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_COOLANT].value = 52;
    s.readings[SIG_OIL].value = 41;
    s.readings[SIG_IAT].value = 18;
    s.readings[SIG_RPM].value = 1450;
    render_screen(c, &s, (screen_t){0});
}

// Oil past the critical temperature takes the whole screen.
static void shot_oil_critical(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_OIL].value = 134;
    render_screen(c, &s, (screen_t){0});
}

static void shot_oil(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 1});
}

// The views the grid leaves out: the pedal, flat out, and the load.
static void shot_pedal(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 7});
}

static void shot_load(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 8});
}

// Near the rev limiter, in the flash's lit half (now is on one).
static void shot_limiter(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_RPM].value = 7450;
    render_screen(c, &s, (screen_t){0});
}

static void shot_coolant(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 2});
}

// Its lowest and highest as wide as they get (13.9V 14.4V): they ran into
// each other once.
static void shot_battery(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 5});
}

const shot_t gauge_shots[] = {
    {"grid", shot_grid},
    {"alarms", shot_alarms},
    {"track", shot_track},
    {"shot_alarms_unlit", shot_alarms_unlit},
    {"oil_critical", shot_oil_critical},
    {"cold", shot_cold},
    {"shift", shot_shift},
    {"oil", shot_oil},
    {"coolant", shot_coolant},
    {"pedal", shot_pedal},
    {"load", shot_load},
    {"battery", shot_battery},
    {"limiter", shot_limiter},
};
const int gauge_shot_count = sizeof gauge_shots / sizeof gauge_shots[0];
