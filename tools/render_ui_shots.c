// The screens render_ui draws: each one a fixed state, as the panel would
// show it.

#include <stdio.h>
#include <string.h>

#include "obd/mcp_check.h"
#include "render_ui.h"

static const uint32_t now = 100000;

// state_while_driving is a session so far (the extremes) and its latest readings.
static state_t state_while_driving(void) {
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

static settings_t cfg = {.brightness = BRIGHTNESS_MAX, .auto_connect = true, .shift_rpm = 7000};

static void render_screen(canvas_t *c, const state_t *s, screen_t screen) {
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

// On track, the stability control off: the track grid and its badge.
static void shot_track(canvas_t *c, drive_mode_t mode) {
    state_t s = state_while_driving();
    s.readings[SIG_OIL].value = 116;
    s.readings[SIG_AFR].value = 12.4f;
    state_set_drive_mode(&s, mode, now);
    render_screen(c, &s, (screen_t){0});
}

static void shot_track_trc(canvas_t *c) {
    shot_track(c, DRIVE_TRC_OFF);
}

static void shot_track_vsc(canvas_t *c) {
    shot_track(c, DRIVE_VSC_OFF);
}

static void shot_track_sport(canvas_t *c) {
    shot_track(c, DRIVE_SPORT_TRC_OFF);
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

// A prompt waiting for the driver's choice, the knob on choice.
static void render_choice_screen(canvas_t *c, const char *text, choice_t choice) {
    state_t s = {.wait = WAIT_CHOICE};
    snprintf(s.status, sizeof s.status, "%s", text);
    render_screen(c, &s, (screen_t){.choice = choice});
}

static void shot_ready(canvas_t *c) {
    render_choice_screen(c, "READY\nIOS-Vlink\n(last used)", CHOICE_CONNECT);
}

// The dongle asleep, the car off: no failure, a click reconnects.
static void shot_asleep(canvas_t *c) {
    render_choice_screen(c, "DISCONNECTED\npress to reconnect", CHOICE_CONNECT);
}

static void shot_offline(canvas_t *c) {
    render_choice_screen(c, "OFFLINE\nIOS-Vlink", CHOICE_SETTINGS);
}

// The screensaver, on the 86's logo (the parts' logos stay out of git):
// the shine across it, then fading in.
static void shot_showcar(canvas_t *c) {
    state_t s = {.wait = WAIT_CHOICE};
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 1300);
}

static void shot_showcar_later(canvas_t *c) {
    state_t s = {.wait = WAIT_CHOICE};
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 300);
}

// The longest title, one word, as ble.c words it.
static void shot_handshake(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY, .status = "HANDSHAKE\n41:42:86:9A:C0:33"};
    render_screen(c, &s, (screen_t){0});
}

static void shot_connecting(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", "LINKING\n41:42:86:9A:C0:33");
    render_screen(c, &s, (screen_t){0});
}

static void shot_failed(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s",
             "NO LINK\nBLE connect failed:\ncan't connect: the radio still holds another connection (0x56)");
    render_screen(c, &s, (screen_t){.choice = CHOICE_SETTINGS});
}
// The MCP2515's self test: its longest failure, and a silent bus.
static void shot_can_failed(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", mcp_check_spi(0x00, false));
    render_screen(c, &s, (screen_t){0});
}

static void shot_can_silent(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", mcp_bus_status(BUS_SILENT));
    render_screen(c, &s, (screen_t){0});
}

// A RAM scan, its Cancel chosen.
static void shot_scan(canvas_t *c) {
    state_t s = {.wait = WAIT_CANCEL, .status = "DEEP SCAN\n37 KB of ECU RAM\n3 candidates"};
    render_screen(c, &s, (screen_t){.choice = CHOICE_CONNECT});
}

static void shot_settings(canvas_t *c) {
    state_t s = {0};
    render_screen(c, &s, (screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SHIFT_LIGHT, .editing = true});
}

// With the CAN source, the rows needing the dongle dimmed.
static void shot_settings_can(canvas_t *c) {
    state_t s = {0};
    settings_t can = cfg;
    can.source = SOURCE_CAN;
    ui_render(c, &s, &(screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SOURCE}, &can, now);
}

// The boot animation: spinning, then closing into the logo.
static void shot_boot_spin(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS / 2, now);
}

static void shot_boot_morph(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS / 2, now);
}

static void shot_boot_logo(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS + 1, now);
}

const shot_t shots[] = {
    {"grid", shot_grid, false},
    {"alarms", shot_alarms, false},
    {"track_trc", shot_track_trc, false},
    {"track_vsc", shot_track_vsc, false},
    {"track_sport", shot_track_sport, false},
    {"shot_alarms_unlit", shot_alarms_unlit, false},
    {"oil_critical", shot_oil_critical, false},
    {"cold", shot_cold, false},
    {"shift", shot_shift, false},
    {"oil", shot_oil, false},
    {"coolant", shot_coolant, false},
    {"pedal", shot_pedal, false},
    {"load", shot_load, false},
    {"limiter", shot_limiter, false},
    {"ready", shot_ready, false},
    {"offline", shot_offline, false},
    {"asleep", shot_asleep, false},
    {"connecting", shot_connecting, false},
    {"scan", shot_scan, false},
    {"failed", shot_failed, false},
    {"can_failed", shot_can_failed, false},
    {"can_silent", shot_can_silent, false},
    {"showcar", shot_showcar, false},
    {"showcar_later", shot_showcar_later, false},
    {"settings", shot_settings, false},
    {"settings_can", shot_settings_can, false},
    {"shot_boot_spin", shot_boot_spin, false},
    {"shot_boot_morph", shot_boot_morph, false},
    {"shot_boot_logo", shot_boot_logo, false},
    // The same screens with the panel on its side.
    {"grid_landscape", shot_grid, true},
    {"alarms_landscape", shot_alarms, true},
    {"track_landscape", shot_track_trc, true},
    {"oil_landscape", shot_oil, true},
    {"oil_critical_landscape", shot_oil_critical, true},
    {"limiter_landscape", shot_limiter, true},
    {"ready_landscape", shot_ready, true},
    {"handshake_landscape", shot_handshake, true},
    {"asleep_landscape", shot_asleep, true},
    {"failed_landscape", shot_failed, true},
    {"scan_landscape", shot_scan, true},
    {"settings_landscape", shot_settings, true},
    {"showcar_landscape", shot_showcar, true},
    {"boot_landscape", shot_boot_morph, true},
};
const int shot_count = sizeof shots / sizeof shots[0];
