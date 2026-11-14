#include "ui.h"

#include <math.h>
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

// Most signals update several times a second. The oil temperature comes
// every 3s through the dongle (ELM_OIL_INTERVAL_MS).
#define FAST_STALE_MS 2000
#define OIL_STALE_MS 10000

// From OIL_CRITICAL, the oil temperature takes the whole screen, on every
// page: the tile alone, red from OIL_RANGE's 120, is easy to miss, and
// 120 comes quickly on a track (the FA20 has no oil cooler).
#define OIL_CRITICAL 130

// No RPM tile: the car's own tachometer is right there. RPM still drives
// the shift light.
const tile_t ui_tiles[] = {
    {"OIL TEMP", "%.0f", DEGREE "C", SIG_OIL, OIL_RANGE, OIL_STALE_MS},
    {"COOLANT", "%.0f", DEGREE "C", SIG_COOLANT, COOLANT_RANGE, FAST_STALE_MS},
    {"AFR", "%.1f", "", SIG_AFR, AFR_RANGE, FAST_STALE_MS},
    {"INTAKE", "%.0f", DEGREE "C", SIG_IAT, IAT_RANGE, FAST_STALE_MS},
    {"BATTERY", "%.1f", "V", SIG_BATTERY, BATTERY_RANGE, FAST_STALE_MS},
    {"ETHANOL", "%.0f", "%", SIG_ETHANOL, ETHANOL_RANGE, FAST_STALE_MS},
};
const int ui_tile_count = sizeof ui_tiles / sizeof ui_tiles[0];

extern const unsigned char logo_bin[];

#define GRID_GAP 3
#define GRID_COLUMNS 2

static const font_t *const label_font = &font_bold9;
// Tried largest first: wider readings ("-14.2V") drop a size or two to
// stay inside their tile.
static const font_t *const value_fonts[] = {&font_bold24, &font_bold18, &font_bold12};

#define PI 3.14159265f

static color_t range_color(const range_t *r, float v, const theme_t *t) {
    if (v < r->cold) {
        return t->cold;
    }
    if (v <= r->danger_low || v >= r->danger_high) {
        return t->danger;
    }
    if (v <= r->warn_low || v >= r->warn_high) {
        return t->warning;
    }
    return t->good;
}

static float range_fraction(const range_t *r, float v) {
    if (r->max <= r->min) {
        return 0;
    }
    float f = (v - r->min) / (r->max - r->min);
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

// tile_value is what tile shows for reading: its text, color and gauge
// fraction, or "--" once stale (fraction -1: nothing lit).
static void tile_value(const tile_t *tile, reading_t reading, uint32_t now_ms, const theme_t *t, char text[24],
                       color_t *color, float *fraction) {
    if (reading_stale(&reading, now_ms, tile->stale_ms)) {
        snprintf(text, 24, "--");
        *color = t->stale;
        *fraction = -1;
        return;
    }
    char number[16];
    snprintf(number, sizeof number, tile->format, (double)reading.value);
    snprintf(text, 24, "%s%s", number, tile->unit);
    *color = range_color(&tile->range, reading.value, t);
    *fraction = range_fraction(&tile->range, reading.value);
}

#define MAX_SEGMENTS 40

// segment_colors colors count gauge segments like a GT86 cluster: lit up
// to fraction, each in the band of the value it stands for, the unlit
// ones a glimmer of that band so the red zone shows at rest. On an alarm
// tile, lit segments take the alarm text color. peak (a fraction, or -1
// for none) marks the session's highest value in the alarm text color.
static void segment_colors(const range_t *r, float fraction, float peak, int count, bool alarm, const theme_t *t,
                           color_t out[]) {
    color_t back = alarm ? t->alarm : t->background;
    int lit = fraction < 0 ? 0 : (int)(fraction * (float)count + 0.5f);
    if (fraction > 0 && lit == 0) {
        lit = 1;
    }
    for (int i = 0; i < count; i++) {
        color_t band = range_color(r, r->min + ((float)i + 0.5f) / (float)count * (r->max - r->min), t);
        if (i < lit) {
            out[i] = alarm ? t->alarm_text : band;
        } else {
            out[i] = gfx_blend(back, band, alarm ? 0.35f : 0.2f);
        }
    }
    if (peak >= 0) {
        int at = (int)(peak * (float)count);
        out[at < count ? at : count - 1] = t->alarm_text;
    }
}

// draw_bar draws count segments in a row.
static void draw_bar(canvas_t *c, int x, int y, int w, int h, int count, const color_t colors[]) {
    const int gap = 2;
    int seg = (w + gap) / count;
    x += (w - (seg * count - gap)) / 2;
    for (int i = 0; i < count; i++) {
        gfx_fill_rect(c, x + i * seg, y, seg - gap, h, colors[i]);
    }
}

// draw_arc draws count segments around a ring centered on cx, cy, from
// start degrees over sweep degrees (clockwise, 0 pointing right), out of
// 3px squares: the panel has no line drawing.
static void draw_arc(canvas_t *c, int cx, int cy, int r_in, int r_out, float start, float sweep, int count,
                     const color_t colors[]) {
    float seg = sweep / (float)count;
    float step = 90.0f / (PI * (float)r_out); // about 1.5px at the rim
    for (int i = 0; i < count; i++) {
        float a0 = start + (float)i * seg;
        for (float a = a0; a <= a0 + seg * 0.75f; a += step) {
            float cs = cosf(a * PI / 180), sn = sinf(a * PI / 180);
            for (int r = r_in; r <= r_out; r += 2) {
                gfx_fill_rect(c, cx + (int)lroundf((float)r * cs) - 1, cy + (int)lroundf((float)r * sn) - 1, 3, 3,
                              colors[i]);
            }
        }
    }
}

static void draw_frame(canvas_t *c, int x, int y, int w, int h, color_t color) {
    gfx_fill_rect(c, x, y, w, 1, color);
    gfx_fill_rect(c, x, y + h - 1, w, 1, color);
    gfx_fill_rect(c, x, y, 1, h, color);
    gfx_fill_rect(c, x + w - 1, y, 1, h, color);
}

static void text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color, int scale) {
    gfx_text(c, font, cx - gfx_text_width(font, s) * scale / 2, y, s, color, scale);
}

// A reading in danger blinks: its tile lit red, then not, twice a second.
#define ALARM_BLINK_MS 250

static bool alarm_lit(uint32_t now_ms) {
    return now_ms / ALARM_BLINK_MS % 2 == 0;
}

// text_fit centers s in the largest font it fits max_width in.
static void text_fit(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color) {
    static const font_t *const fonts[] = {&font_bold24, &font_bold18, &font_bold12, &font_bold9};
    int i = 0;
    while (i < 3 && gfx_text_width(fonts[i], s) > max_width) {
        i++;
    }
    text_centered(c, fonts[i], cx, y, s, color, 1);
}

#define BAR_SEGMENTS 12

static void draw_tile(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, reading_t r,
                      uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    tile_value(tile, r, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    draw_frame(c, x, y, w, h, color == t->danger ? t->danger : t->frame);
    int label_y = y + 17;
    gfx_text(c, label_font, x + 8, label_y, tile->label, alarm ? t->alarm_text : t->label, 1);

    const font_t *font = value_fonts[2];
    for (int i = 0; i < 3; i++) {
        if (gfx_text_width(value_fonts[i], text) <= w - 16) {
            font = value_fonts[i];
            break;
        }
    }
    gfx_text(c, font, x + 8, label_y + 42, text, alarm ? t->alarm_text : color, 1);
    color_t colors[BAR_SEGMENTS];
    segment_colors(&tile->range, fraction, -1, BAR_SEGMENTS, alarm, t, colors);
    draw_bar(c, x + 8, y + h - 16, w - 16, 8, BAR_SEGMENTS, colors);
}

#define DIAL_SEGMENTS 30
#define DIAL_START 135
#define DIAL_SWEEP 270
#define DIAL_WIDTH 14

// draw_tile_large draws a tile alone, as a round dial like the GT86's
// tachometer: segments around the value, blown up as much as fits
// inside, up to 3 times (chunky, but readable from the seat, without a
// bigger font taking flash).
static void draw_tile_large(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h,
                            reading_t r, uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    tile_value(tile, r, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    int cx = x + w / 2, r_out = w / 2 - 4, cy = y + r_out + 8, r_in = r_out - DIAL_WIDTH;
    color_t colors[DIAL_SEGMENTS];
    float peak = r.valid ? range_fraction(&tile->range, r.max) : -1;
    segment_colors(&tile->range, fraction, peak, DIAL_SEGMENTS, alarm, t, colors);
    draw_arc(c, cx, cy, r_in, r_out, DIAL_START, DIAL_SWEEP, DIAL_SEGMENTS, colors);

    int width = gfx_text_width(&font_bold24, text);
    int scale = 3;
    while (scale > 1 && width * scale > 2 * r_in - 24) {
        scale--;
    }
    // The 24pt digits rise about 26px above their baseline.
    const int cap_height = 26;
    text_centered(c, &font_bold24, cx, cy + cap_height * scale / 2, text, alarm ? t->alarm_text : color, scale);

    // The scale's ends, under the ring's opening, and the label below.
    char end[16];
    int ends_y = cy + r_out * 7 / 10 + 22, ends_dx = r_out * 7 / 10;
    color_t dim = alarm ? t->alarm_text : t->label;
    snprintf(end, sizeof end, tile->format, (double)tile->range.min);
    text_centered(c, label_font, cx - ends_dx, ends_y, end, dim, 1);
    snprintf(end, sizeof end, tile->format, (double)tile->range.max);
    text_centered(c, label_font, cx + ends_dx, ends_y, end, dim, 1);
    // The session's extremes, to read back after a run.
    if (r.valid) {
        char low[16], high[16], line[48];
        snprintf(low, sizeof low, tile->format, (double)r.min);
        snprintf(high, sizeof high, tile->format, (double)r.max);
        snprintf(line, sizeof line, "MIN %s%s   MAX %s%s", low, tile->unit, high, tile->unit);
        text_centered(c, label_font, cx, ends_y + 30, line, alarm ? t->alarm_text : t->label, 1);
    }
    text_centered(c, &font_bold12, cx, y + h - 14, tile->label, dim, 1);
}

// draw_grid draws every tile, from top down.
static void draw_grid(canvas_t *c, const theme_t *t, const reading_t *readings, int top, uint32_t now_ms) {
    int rows = (ui_tile_count + GRID_COLUMNS - 1) / GRID_COLUMNS;
    int cell_w = c->width / GRID_COLUMNS, cell_h = (c->height - top) / rows;
    for (int i = 0; i < ui_tile_count; i++) {
        int col = i % GRID_COLUMNS, row = i / GRID_COLUMNS;
        draw_tile(c, t, &ui_tiles[i], col * cell_w + GRID_GAP, top + row * cell_h + GRID_GAP,
                  cell_w - 2 * GRID_GAP, cell_h - 2 * GRID_GAP, readings[ui_tiles[i].signal], now_ms);
    }
}

int ui_wrap(const font_t *font, const char *s, int max_width, char out[][64], int max) {
    int n = 0;
    char line[64] = "";
    while (*s && n < max) {
        while (*s == ' ') {
            s++;
        }
        size_t word_len = strcspn(s, " ");
        if (word_len == 0) {
            break;
        }
        char candidate[64];
        if (line[0]) {
            snprintf(candidate, sizeof candidate, "%s %.*s", line, (int)word_len, s);
        } else {
            snprintf(candidate, sizeof candidate, "%.*s", (int)word_len, s);
        }
        if (line[0] && gfx_text_width(font, candidate) > max_width) {
            snprintf(out[n++], 64, "%s", line);
            snprintf(line, sizeof line, "%.*s", (int)word_len, s);
        } else {
            snprintf(line, sizeof line, "%s", candidate);
        }
        s += word_len;
    }
    if (n < max && (line[0] || n == 0)) {
        snprintf(out[n++], 64, "%s", line);
    }
    return n;
}

static void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color);

#define STATUS_MARGIN 8
#define STATUS_LINE_HEIGHT 20
#define RING_SEGMENTS 24
#define RING_Y 100
#define RING_OUT 78
#define RING_IN 68

// draw_status is the source's report while it isn't delivering: the
// logo in a ring whose lit head goes round (a comet tail behind it, a
// step every 80ms, so a slow step doesn't look frozen), the first
// paragraph as a title, the rest centered and wrapped under it.
static void draw_status(canvas_t *c, const theme_t *t, const char *status, uint32_t now_ms) {
    int head = (int)(now_ms / 80 % RING_SEGMENTS);
    color_t colors[RING_SEGMENTS];
    for (int i = 0; i < RING_SEGMENTS; i++) {
        int age = (head - i + RING_SEGMENTS) % RING_SEGMENTS;
        colors[i] = age < 8 ? gfx_blend(t->accent, t->stale, (float)age / 8) : t->stale;
    }
    draw_arc(c, c->width / 2, RING_Y, RING_IN, RING_OUT, -90, 360, RING_SEGMENTS, colors);
    draw_logo(c, c->width / 2, RING_Y, 2, t->accent);

    int y = RING_Y + RING_OUT + 34;
    bool first = true;
    char paragraph[192];
    snprintf(paragraph, sizeof paragraph, "%s", status);
    for (char *p = paragraph; p && y < c->height; first = false) {
        char *next = strchr(p, '\n');
        if (next) {
            *next++ = 0;
        }
        const font_t *font = first ? &font_bold12 : label_font;
        char lines[8][64];
        int n = ui_wrap(font, p, c->width - 2 * STATUS_MARGIN, lines, 8);
        for (int i = 0; i < n; i++) {
            text_centered(c, font, c->width / 2, y, lines[i], first ? t->accent : t->label, 1);
            y += STATUS_LINE_HEIGHT + (first ? 6 : 0);
        }
        p = next;
    }
}

#define SETTINGS_MARGIN 8
#define SETTINGS_TOP 46
#define SETTINGS_ROW_HEIGHT 31

static void draw_settings(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg) {
    gfx_text(c, &font_bold12, SETTINGS_MARGIN + 4, 30, "SETTINGS", t->accent, 1);
    gfx_fill_rect(c, SETTINGS_MARGIN, 38, c->width - 2 * SETTINGS_MARGIN, 1, t->frame);
    char brightness[8], shift[8];
    snprintf(brightness, sizeof brightness, "%d%%", cfg->brightness);
    snprintf(shift, sizeof shift, cfg->shift_rpm ? "%d" : "OFF", cfg->shift_rpm);
    const char *rows[SETTINGS_ROWS][2] = {
        [ROW_BRIGHTNESS] = {"Brightness", brightness},
        [ROW_SHIFT_LIGHT] = {"Shift light", shift},
        [ROW_AUTO_CONNECT] = {"Auto-connect", cfg->auto_connect ? "ON" : "OFF"},
        [ROW_FLIP] = {"Flip screen", cfg->flip ? "ON" : "OFF"},
        [ROW_ETHANOL_SENSOR] = {"Ethanol sensor", cfg->ethanol_sensor ? "ON" : "OFF"},
        [ROW_SCAN] = {"Scan ECU RAM", ""},
        [ROW_LISTEN] = {"Listen CAN", ""},
        [ROW_BACK] = {"Back", ""},
    };
    for (int i = 0; i < SETTINGS_ROWS; i++) {
        int y = SETTINGS_TOP + i * SETTINGS_ROW_HEIGHT;
        bool off = strcmp(rows[i][1], "OFF") == 0;
        color_t label = t->label, value = off ? t->frame : t->accent;
        if (i == screen->cursor) {
            int h = SETTINGS_ROW_HEIGHT - 6;
            draw_frame(c, SETTINGS_MARGIN, y, c->width - 2 * SETTINGS_MARGIN, h, t->accent);
            gfx_fill_rect(c, SETTINGS_MARGIN, y, 4, h, t->accent);
            label = t->alarm_text;
            if (off) {
                value = t->label;
            }
            if (screen->editing) {
                value = t->warning;
            }
        }
        gfx_text(c, label_font, SETTINGS_MARGIN + 12, y + 18, rows[i][0], label, 1);
        int width = gfx_text_width(label_font, rows[i][1]);
        gfx_text(c, label_font, c->width - SETTINGS_MARGIN - 10 - width, y + 18, rows[i][1], value, 1);
    }
    text_centered(c, label_font, c->width / 2, c->height - 14, "hold: reboot", t->frame, 1);
}

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
    bool fresh = !reading_stale(rpm, now_ms, FAST_STALE_MS);
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

// draw_oil_critical is the whole screen for an oil temperature at
// OIL_CRITICAL or more, blinking, in the driver's words: calm down, it's
// getting hot. Lifting off and driving gently cools the oil; shutting the
// engine off would stop it flowing.
static void draw_oil_critical(canvas_t *c, const theme_t *t, float oil, uint32_t now_ms) {
    bool lit = alarm_lit(now_ms);
    color_t text = lit ? t->alarm_text : t->danger;
    gfx_fill_rect(c, 0, 0, c->width, c->height, lit ? t->alarm : t->background);
    text_centered(c, &font_bold24, c->width / 2, 48, "OIL TEMP", text, 1);

    // Degrees only: with the C it wouldn't fit 3 times as big.
    char value[16];
    snprintf(value, sizeof value, "%.0f" DEGREE, (double)oil);
    int scale = 3;
    while (scale > 1 && gfx_text_width(&font_bold24, value) * scale > c->width - 16) {
        scale--;
    }
    text_centered(c, &font_bold24, c->width / 2, 158 + 13 * scale, value, text, scale);
    // The fonts are ASCII only: no accents.
    text_fit(c, c->width / 2, 266, c->width - 16, "CALME-TOI", text);
    text_fit(c, c->width / 2, 304, c->width - 16, "CA CHAUFFE SA MERE", text);
}

void ui_render(canvas_t *c, const state_t *state, const screen_t *screen, const settings_t *cfg, uint32_t now_ms) {
    const theme_t *t = &theme_dark;
    const reading_t *oil = &state->readings[SIG_OIL];
    if (!reading_stale(oil, now_ms, OIL_STALE_MS) && oil->value >= OIL_CRITICAL) {
        draw_oil_critical(c, t, oil->value, now_ms);
        return;
    }
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (screen->page == PAGE_SETTINGS) {
        draw_settings(c, t, screen, cfg);
        return;
    }
    if (state->status[0]) {
        draw_status(c, t, state->status, now_ms);
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
        draw_tile_large(c, t, tile, GRID_GAP, top + GRID_GAP, c->width - 2 * GRID_GAP,
                        c->height - top - 2 * GRID_GAP, state->readings[tile->signal], now_ms);
    } else {
        draw_grid(c, t, state->readings, top, now_ms);
    }
}

// draw_logo draws the logo centered on cx, cy, shrink times smaller: a
// pixel is lit if any in its shrink x shrink block is.
static void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color) {
    int w = logo_bin[0] << 8 | logo_bin[1], h = logo_bin[2] << 8 | logo_bin[3];
    int stride = (w + 7) / 8;
    int x0 = cx - w / shrink / 2, y0 = cy - h / shrink / 2;
    const unsigned char *rows = logo_bin + 4;
    for (int y = 0; y < h / shrink; y++) {
        for (int x = 0; x < w / shrink; x++) {
            bool on = false;
            for (int k = 0; k < shrink * shrink && !on; k++) {
                int sx = x * shrink + k % shrink, sy = y * shrink + k / shrink;
                on = rows[sy * stride + sx / 8] & (0x80 >> (sx % 8));
            }
            if (on) {
                gfx_fill_rect(c, x0 + x, y0 + y, 1, 1, color);
            }
        }
    }
}

// The boot shift light revs over the bar's span up to this.
#define BOOT_SHIFT_RPM 7000

bool ui_render_boot(canvas_t *c, uint32_t elapsed_ms, uint32_t now_ms) {
    if (elapsed_ms >= BOOT_DURATION_MS) {
        return false;
    }
    const theme_t *t = &theme_dark;
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    reading_t rpm = {.valid = true, .updated_ms = now_ms};
    if (elapsed_ms < BOOT_SPLASH_MS) {
        // The logo, revealed top down behind a bright scan line.
        int top = (c->height - (logo_bin[2] << 8 | logo_bin[3])) / 2, bottom = c->height - top;
        draw_logo(c, c->width / 2, c->height / 2, 1, t->accent);
        if (elapsed_ms < BOOT_REVEAL_MS) {
            int line = top + (bottom - top) * (int)elapsed_ms / BOOT_REVEAL_MS;
            gfx_fill_rect(c, 0, line, c->width, c->height - line, t->background);
            gfx_fill_rect(c, 0, line, c->width, 3, t->alarm_text);
            gfx_fill_rect(c, 0, line + 3, c->width, 8, gfx_blend(t->background, t->accent, 0.35f));
        }
        // Then a rev: the shift light fills, then flashes blue.
        float rev = (float)((int)elapsed_ms - BOOT_REVEAL_MS) / (BOOT_SPLASH_MS - BOOT_REVEAL_MS - 200);
        rpm.value = rev < 0 ? 0 : BOOT_SHIFT_RPM - SHIFT_SPAN_RPM + rev * SHIFT_SPAN_RPM;
        draw_shift_light(c, t, &rpm, BOOT_SHIFT_RPM, now_ms);
        return true;
    }
    // Like the GT86's own cluster at ignition: the tachometer sweeps to
    // its end and back, its scale around, the logo in the middle.
    float sweep = sinf(PI * (float)(elapsed_ms - BOOT_SPLASH_MS) / BOOT_SWEEP_MS);
    const range_t tach = RPM_RANGE;
    int cx = c->width / 2, cy = SHIFT_BAR_HEIGHT + 140, r_out = c->width / 2 - 8, r_in = r_out - DIAL_WIDTH;
    color_t colors[DIAL_SEGMENTS];
    segment_colors(&tach, sweep, -1, DIAL_SEGMENTS, false, t, colors);
    draw_arc(c, cx, cy, r_in, r_out, DIAL_START, DIAL_SWEEP, DIAL_SEGMENTS, colors);
    for (int k = 0; k <= 8; k++) {
        float a = (DIAL_START + (float)k * DIAL_SWEEP / 8) * PI / 180;
        char digit[2] = {(char)('0' + k), 0};
        int r = r_in - 16;
        text_centered(c, &font_bold12, cx + (int)lroundf((float)r * cosf(a)), cy + (int)lroundf((float)r * sinf(a)) + 7,
                      digit, k * 1000 >= (int)tach.danger_high ? t->danger : t->label, 1);
    }
    draw_logo(c, cx, cy, 2, t->accent);
    text_centered(c, label_font, cx, cy + r_out * 7 / 10 + 26, "x1000 r/min", t->label, 1);
    rpm.value = BOOT_SHIFT_RPM - SHIFT_SPAN_RPM + sweep * SHIFT_SPAN_RPM * 0.99f;
    draw_shift_light(c, t, &rpm, BOOT_SHIFT_RPM, now_ms);
    return true;
}
