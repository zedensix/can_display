#include "ui_internal.h"

#include <stdio.h>
#include <string.h>

// The GT86's own cluster: black, lit red-orange, with white labels. The
// readings take their band's color: green fine, amber warns, red danger,
// which blinks its whole tile.
const theme_t theme_dark = {
    .background = RGB(0x00, 0x00, 0x00),
    .frame = RGB(0x4a, 0x12, 0x08),
    .accent = RGB(0xff, 0x5a, 0x1e),
    .label = RGB(0xf0, 0xf0, 0xf0),
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

// From OIL_CRITICAL, the oil temperature takes the whole screen, on every
// page: the tile alone, red from OIL_RANGE's 120, is easy to miss, and
// 120 comes quickly on a track (the FA20 has no oil cooler).
#define OIL_CRITICAL 130

// No RPM tile: the car's own tachometer is right there. RPM still drives
// the shift light.
const tile_t ui_tiles[] = {
    {"OIL TEMP", "%.0f", DEGREE "C", SIG_OIL, OIL_RANGE, SLOW_STALE_MS},
    {"COOLANT", "%.0f", DEGREE "C", SIG_COOLANT, COOLANT_RANGE, FAST_STALE_MS},
    {"AFR", "%.1f", "", SIG_AFR, AFR_RANGE, FAST_STALE_MS},
    {"INTAKE", "%.0f", DEGREE "C", SIG_IAT, IAT_RANGE, FAST_STALE_MS},
    {"BATTERY", "%.1f", "V", SIG_BATTERY, BATTERY_RANGE, FAST_STALE_MS},
    {"ETHANOL", "%.0f", "%", SIG_ETHANOL, ETHANOL_RANGE, SLOW_STALE_MS},
};
const int ui_tile_count = sizeof ui_tiles / sizeof ui_tiles[0];
const int ui_view_count = sizeof ui_tiles / sizeof ui_tiles[0];

// The shift light is a race car steering wheel's LED bar across the top:
// SHIFT_LEDS / 2 pairs light from both ends towards the middle over the
// last SHIFT_SPAN_RPM before shift_rpm, green, then amber, then red; from
// shift_rpm on, they all flash blue. Narrow LEDs, close together.
#define SHIFT_BAR_HEIGHT 20
#define SHIFT_LEDS 20
#define SHIFT_SPAN_RPM 1500
#define SHIFT_BLINK_MS 60

static void draw_shift_light(canvas_t *c, const theme_t *t, const reading_t *rpm, int shift_rpm, uint32_t now_ms) {
    const int pairs = SHIFT_LEDS / 2, gap = 2, margin = GRID_GAP;
    int led_w = (c->width - 2 * margin - (SHIFT_LEDS - 1) * gap) / SHIFT_LEDS;
    // What the integer LED width leaves over goes to the outer margins.
    int x0 = (c->width - SHIFT_LEDS * led_w - (SHIFT_LEDS - 1) * gap) / 2;
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
            color = now_ms / SHIFT_BLINK_MS % 2 ? t->background : t->cold;
        } else if (k >= lit) {
            color = gfx_blend(t->background, color, 0.15f);
        }
        gfx_fill_rect(c, x0 + k * (led_w + gap), 3, led_w, SHIFT_BAR_HEIGHT - 6, color);
        gfx_fill_rect(c, c->width - x0 - led_w - k * (led_w + gap), 3, led_w, SHIFT_BAR_HEIGHT - 6, color);
    }
}

// draw_oil_overheat_screen is the whole screen for an oil temperature at
// OIL_CRITICAL or more, blinking, in the driver's words: calm down, it's
// getting hot. Lifting off and driving gently cools the oil; shutting the
// engine off would stop it flowing.
static void draw_oil_overheat_screen(canvas_t *c, const theme_t *t, float oil, uint32_t now_ms) {
    bool lit = alarm_blink_lit(now_ms);
    color_t text = lit ? t->alarm_text : t->danger;
    gfx_fill_rect(c, 0, 0, c->width, c->height, lit ? t->alarm : t->background);
    draw_text_centered(c, &font_bold24, c->width / 2, 48, "OIL TEMP", text, 1);

    // Degrees only: with the C it wouldn't fit 3 times as big.
    char value[16];
    snprintf(value, sizeof value, "%.0f" DEGREE, (double)oil);
    int scale = 3;
    while (scale > 1 && gfx_text_width(&font_bold24, value) * scale > c->width - 16) {
        scale--;
    }
    draw_text_centered(c, &font_bold24, c->width / 2, 158 + 13 * scale, value, text, scale);
    // The fonts are ASCII only: no accents.
    draw_text_fitting(c, c->width / 2, 266, c->width - 16, "TOO HOT", text);
    draw_text_fitting(c, c->width / 2, 304, c->width - 16, "LIFT OFF", text);
}

static bool oil_is_overheating(const state_t *state, uint32_t now_ms) {
    const reading_t *oil = &state->readings[SIG_OIL];
    return !reading_is_stale(oil, now_ms, SLOW_STALE_MS) && oil->value >= OIL_CRITICAL;
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
    // With the shift light on, its bar takes the top of the gauges.
    int top = 0;
    if (cfg->shift_rpm) {
        draw_shift_light(c, t, &state->readings[SIG_RPM], cfg->shift_rpm, now_ms);
        top = SHIFT_BAR_HEIGHT;
    }
    if (screen->view > 0 && screen->view <= ui_tile_count) {
        const tile_t *tile = &ui_tiles[screen->view - 1];
        draw_tile_dial(c, t, tile, GRID_GAP, top + GRID_GAP, c->width - 2 * GRID_GAP, c->height - top - 2 * GRID_GAP,
                       state, now_ms);
    } else if (state_drive_mode(state, now_ms) != DRIVE_NORMAL) {
        draw_track_grid(c, t, state, top, state_drive_mode(state, now_ms), now_ms);
    } else {
        draw_grid(c, t, state, top, now_ms);
    }
}
