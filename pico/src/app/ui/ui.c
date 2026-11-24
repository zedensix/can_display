#include "ui_internal.h"

#include <stdio.h>
#include <string.h>

// The GT86's own cluster, as the Android app draws it (Gauges.kt's
// Colors): black, lit red-orange, with white labels. The readings take
// their band's color: green fine, amber warns, red danger, which blinks
// its whole row.
const theme_t theme_dark = {
    .background = RGB(0x00, 0x00, 0x00),
    .frame = RGB(0x4a, 0x12, 0x08),
    .accent = RGB(0xff, 0x5a, 0x1e),
    .label = RGB(0xf0, 0xf0, 0xf0),
    .dim = RGB(0x9a, 0x9a, 0x9a),
    .good = RGB(0x3c, 0xe0, 0x5a),
    .warning = RGB(0xff, 0xb4, 0x1e),
    .danger = RGB(0xff, 0x20, 0x10),
    .alarm = RGB(0xb8, 0x10, 0x08),
    .alarm_text = RGB(0xff, 0xf0, 0xe0),
    // The one cold color in the cluster, like the GT86's own cold engine
    // lamp: don't rev it yet.
    .cold = RGB(0x2e, 0x8c, 0xff),
    .stale = RGB(0x3a, 0x10, 0x08),
};

// Scale, warning and danger bands per gauge, then the cold band (see
// range_t). Cold is no alarm: oil and coolant start there every morning.
// Oil warms up slower than the coolant, and it's what matters before
// revving hard.
// The tune raised the rev limiter to 7600 (stock: 7450): red from there,
// amber 500 before.
#define RPM_RANGE {0, 8000, -1, 7100, -1, 7600, -1}
// Intake air: ambient plus engine bay heat; past 45 the engine loses
// power, past 60 it's heat-soaked.
#define IAT_RANGE {-10, 80, -1, 45, -1, 60, -1}
#define COOLANT_RANGE {40, 130, -1, 105, -1, 115, 70}
#define OIL_RANGE {40, 150, -1, 110, -1, 120, 80}
#define BATTERY_RANGE {10, 16, 12, 15, 11.5f, 15.5f, -1}
#define AFR_RANGE {10, 18, 12, 16, 11, 17, -1}
#define ETHANOL_RANGE {0, 100, -1, 101, -1, 101, -1}
// The pedal and the load: no wrong value, green all the way.
#define PERCENT_RANGE {0, 100, -1, 101, -1, 101, -1}

// From LIMITER_FLASH_RPM, near the rev limiter (RPM_RANGE's 7600), the
// whole gauges page flashes: the shift light's LEDs are at the edge of
// the eye, this isn't. Flashing every LIMITER_FLASH_MS.
#define LIMITER_FLASH_RPM 7300
#define LIMITER_FLASH_MS 100

// From OIL_CRITICAL, the oil temperature takes the whole screen, on every
// page: the tile alone, red from OIL_RANGE's 120, is easy to miss, and
// 120 comes quickly on a track (the FA20 has no oil cooler).
#define OIL_CRITICAL 130

// No RPM tile: the car's own tachometer is right there. RPM still drives
// the shift light.
const tile_t ui_tiles[] = {
    {"OIL TEMP", "OIL", "%.0f", DEGREE "C", SIG_OIL, OIL_RANGE, SLOW_STALE_MS},
    {"COOLANT", "COOLANT", "%.0f", DEGREE "C", SIG_COOLANT, COOLANT_RANGE, SLOW_STALE_MS},
    {"AFR", "AFR", "%.1f", "", SIG_AFR, AFR_RANGE, FAST_STALE_MS},
    {"INTAKE", "INTAKE", "%.0f", DEGREE "C", SIG_IAT, IAT_RANGE, FAST_STALE_MS},
    {"BATTERY", "BATTERY", "%.1f", "V", SIG_BATTERY, BATTERY_RANGE, FAST_STALE_MS},
    {"ETHANOL", "ETHANOL", "%.0f", "%", SIG_ETHANOL, ETHANOL_RANGE, SLOW_STALE_MS},
    // Views only, past the grid's tiles.
    {"PEDAL", "PEDAL", "%.0f", "%", SIG_PEDAL, PERCENT_RANGE, FAST_STALE_MS},
    {"LOAD", "LOAD", "%.0f", "%", SIG_LOAD, PERCENT_RANGE, FAST_STALE_MS},
};
const int ui_tile_count = 6;
const int ui_view_count = sizeof ui_tiles / sizeof ui_tiles[0];

// The shift light is a race car steering wheel's LED bar across the top:
// SHIFT_LEDS / 2 pairs light from both ends towards the middle over the
// last SHIFT_SPAN_RPM before shift_rpm, green, then amber, then red; from
// shift_rpm on, they all flash blue. Narrow LEDs, close together.
#define SHIFT_LEDS 20
#define SHIFT_SPAN_RPM 1500
#define SHIFT_BLINK_MS 60

static void draw_shift_light(canvas_t *c, const theme_t *t, const reading_t *rpm, int shift_rpm, uint32_t now_ms) {
    const int pairs = SHIFT_LEDS / 2, gap = 2;
    float led = (float)(c->width - 8 - (SHIFT_LEDS - 1) * gap) / SHIFT_LEDS;
    bool fresh = !reading_is_stale(rpm, now_ms, FAST_STALE_MS);
    bool shift = fresh && rpm->value >= (float)shift_rpm;
    int lit = 0;
    if (fresh && !shift) {
        float from = (float)(shift_rpm - SHIFT_SPAN_RPM);
        lit = rpm->value < from ? 0 : 1 + (int)((rpm->value - from) / ((float)SHIFT_SPAN_RPM / pairs));
    }
    for (int k = 0; k < pairs; k++) {
        color_t color = k < 4 ? t->good : k < 7 ? t->warning : t->danger;
        if (shift) {
            color = now_ms / SHIFT_BLINK_MS % 2 ? gfx_blend(t->background, t->cold, 0.1f) : t->cold;
        } else if (k >= lit) {
            color = gfx_blend(t->background, color, 0.14f);
        }
        float left = 4 + (float)k * (led + gap), right = (float)c->width - 4 - led - (float)k * (led + gap);
        for (int side = 0; side < 2; side++) {
            float x = side ? right : left;
            float xs[] = {x, x + led, x + led, x}, ys[] = {3, 3, 10, 10};
            gfx_fill_polygon(c, xs, ys, 4, color);
        }
    }
}

// draw_oil_overheat_screen is the whole screen for an oil temperature at
// OIL_CRITICAL or more, blinking, in the driver's words: calm down, it's
// getting hot. Lifting off and driving gently cools the oil; shutting the
// engine off would stop it flowing. Laid out for 240 high, centered.
static void draw_oil_overheat_screen(canvas_t *c, const theme_t *t, float oil, uint32_t now_ms) {
    bool lit = now_ms / 400 % 2 == 0;
    color_t ink = lit ? t->alarm_text : t->danger;
    gfx_fill_rect(c, 0, 0, c->width, c->height, lit ? t->alarm : t->background);
    int cx = c->width / 2, y = (c->height - 240) / 2;
    draw_text_centered(c, &font_value, cx, y + 56, "OIL TEMP", ink, 4);
    char value[16];
    snprintf(value, sizeof value, "%.0f", (double)oil);
    int wv = gfx_text_width(&font_huge, value, 0), wd = gfx_text_width(&font_large, DEGREE, 0);
    int left = cx - (wv + wd) / 2;
    gfx_text(c, &font_huge, left, y + 163, value, ink, 0);
    gfx_text(c, &font_large, left + wv, y + 163 - font_huge.cap + font_large.cap, DEGREE, ink, 0);
    if (is_landscape(c)) {
        draw_text_centered(c, &font_value, cx, y + 204, "TOO HOT, LIFT OFF", ink, 2);
        return;
    }
    draw_text_centered(c, &font_value, cx, y + 210, "TOO HOT,", ink, 2);
    draw_text_centered(c, &font_value, cx, y + 240, "LIFT OFF", ink, 2);
}

static bool oil_is_overheating(const state_t *state, uint32_t now_ms) {
    const reading_t *oil = &state->readings[SIG_OIL];
    return !reading_is_stale(oil, now_ms, SLOW_STALE_MS) && oil->value >= OIL_CRITICAL;
}

// draw_limiter_flash is the whole screen, every other LIMITER_FLASH_MS, at
// LIMITER_FLASH_RPM or more: red, SHIFT across it, as big as fits.
static void draw_limiter_flash(canvas_t *c, const theme_t *t, const reading_t *rpm, uint32_t now_ms) {
    if (reading_is_stale(rpm, now_ms, FAST_STALE_MS) || rpm->value < LIMITER_FLASH_RPM ||
        now_ms / LIMITER_FLASH_MS % 2) {
        return;
    }
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->alarm);
    const font_t *font = gfx_text_width(&font_huge, "SHIFT", 0) <= c->width - 16 ? &font_huge : &font_large;
    int spacing = gfx_text_width(font, "SHIFT", 6) <= c->width - 16 ? 6 : 0;
    draw_text_centered(c, font, c->width / 2, (c->height + font->cap) / 2, "SHIFT", t->alarm_text, spacing);
}

bool ui_shows_status_screen(const state_t *state, const screen_t *screen, uint32_t now_ms) {
    return !oil_is_overheating(state, now_ms) && !screen->idle && screen->page != PAGE_SETTINGS && state->status[0];
}

void ui_render(canvas_t *c, const state_t *state, const screen_t *screen, const settings_t *cfg, uint32_t now_ms) {
    const theme_t *t = &theme_dark;
    if (oil_is_overheating(state, now_ms)) {
        draw_oil_overheat_screen(c, t, state->readings[SIG_OIL].value, now_ms);
        return;
    }
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (screen->page == PAGE_SETTINGS) {
        draw_settings_page(c, t, screen, cfg);
        return;
    }
    if (screen->idle) {
        draw_showcar(c, t, now_ms);
        return;
    }
    if (state->status[0]) {
        draw_status_screen(c, t, state, screen, now_ms);
        return;
    }
    // A door or the boot open takes the gauges page, after the oil, until
    // dismissed.
    if (screen_shows_doors(screen, state, now_ms)) {
        draw_door_screen(c, t, state_doors(state, now_ms));
        return;
    }
    // With the shift light on, its bar takes the top of the gauges.
    int top = 0;
    if (cfg->shift_rpm) {
        draw_shift_light(c, t, &state->readings[SIG_RPM], cfg->shift_rpm, now_ms);
        top = SHIFT_BAR_HEIGHT;
    }
    int bottom = c->height - INFO_STRIP_HEIGHT;
    if (screen->view > 0 && screen->view <= ui_view_count) {
        // A gauge alone, the whole page under the shift light.
        draw_alone_page(c, t, &ui_tiles[screen->view - 1], top, c->height, state, now_ms);
    } else if (state_drive_mode(state, now_ms) != DRIVE_COMFORT) {
        draw_track_grid(c, t, state, top, bottom, state_drive_mode(state, now_ms), now_ms);
        draw_info_strip(c, t, state, false, now_ms);
    } else {
        draw_main_layout(c, t, state, top, bottom, now_ms);
        draw_info_strip(c, t, state, true, now_ms);
    }
    draw_limiter_flash(c, t, &state->readings[SIG_RPM], now_ms);
}
