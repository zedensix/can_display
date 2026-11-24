// The gauges page's every-gauge view and its track layout, the Android
// app's (Pages.kt street and track): the oil as a dial with its session's
// highest, the other gauges as rows (label, value and unit: the value's
// color says its band; on a 2" panel, bars that small didn't read), the
// oil's dial on the left in landscape, across the top in portrait. On track, the mode's badge, the oil's and the AFR's
// dials, the gear big between or under them, the coolant and the intake
// as rows. A reading in danger blinks its row red.

#include <stdio.h>

#include "ui_internal.h"

// A row is its value's height; its alarm reaches ROW_PAD past it.
#define ROW_HEIGHT 24
#define ROW_PAD 2
// The units' column, right of the values.
#define UNIT_W 17

static const dial_style_t street_style = {10, 40, &font_label, &font_big, &font_text, &font_small};
static const dial_style_t track_style = {8, 32, &font_label, &font_large, &font_small, &font_small};
static const dial_style_t track_portrait_style = {8, 32, &font_label, &font_value, &font_small, &font_small};

// draw_row draws tile as a row from x, y, w wide, ROW_HEIGHT high.
static void draw_row(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, const state_t *s,
                     uint32_t now_ms) {
    shown_t shown = tile_shown(tile, s, now_ms, t);
    // Lit, a row in danger turns red, its text light on it.
    bool alarm = shown.danger && alarm_blink_lit(now_ms);
    if (alarm) {
        fill_round_rect(c, x - ROW_PAD, y - ROW_PAD, w + 2 * ROW_PAD, ROW_HEIGHT + 2 * ROW_PAD, 4, t->alarm,
                        t->background);
    }
    int baseline = y + ROW_HEIGHT;
    gfx_text(c, &font_label, x, baseline, tile->label, alarm ? t->alarm_text : t->label, 1);
    gfx_text(c, &font_small, x + w - UNIT_W + 2, baseline - font_value.cap + font_small.cap,
             shown.fraction >= 0 ? tile->unit : "", alarm ? t->alarm_text : t->dim, 0);
    draw_text_right(c, &font_value, x + w - UNIT_W, baseline, shown.number, alarm ? t->alarm_text : shown.color, 0);
}

static void max_line(const tile_t *tile, const char *word, const state_t *s, char out[24]) {
    const reading_t *r = &s->readings[tile->signal];
    out[0] = 0;
    if (r->valid) {
        char number[16];
        tile_number(tile, r->max, number);
        snprintf(out, 24, "%s %s", word, number);
    }
}

void draw_main_layout(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, uint32_t now_ms) {
    const tile_t *oil = &ui_tiles[0];
    char max[24];
    max_line(oil, "MAX", s, max);
    // ui_tiles' first is the oil; the rest of the grid's tiles, the rows.
    int rows = ui_tile_count - 1, avail = bottom - top;
    if (is_landscape(c)) {
        int size = avail - 4 < 166 ? avail - 4 : 166, y = top + (avail - size) / 2;
        draw_dial(c, t, oil, 0, y, size, &street_style, s, max, false, now_ms);
        int x = size + 8, step = avail / rows < 40 ? avail / rows : 40;
        int y0 = top + (avail - ((rows - 1) * step + ROW_HEIGHT)) / 2;
        for (int i = 0; i < rows; i++) {
            draw_row(c, t, &ui_tiles[1 + i], x, y0 + i * step, c->width - x - 2, s, now_ms);
        }
        return;
    }
    // Rows just far enough apart for their alarm: room for the dial.
    // The rows start in the dial's opening, its last tenth empty.
    int step = ROW_HEIGHT + 3, size = (avail - ((rows - 1) * step + ROW_HEIGHT) - 6) * 10 / 9;
    size = size < c->width - 20 ? size : c->width - 20;
    int x = (c->width - size) / 2;
    draw_dial(c, t, oil, x, top, size, &street_style, s, max, false, now_ms);
    int y0 = top + size * 9 / 10 + 4;
    step = (bottom - 2 - ROW_HEIGHT - y0) / (rows - 1);
    for (int i = 0; i < rows; i++) {
        draw_row(c, t, &ui_tiles[1 + i], 8, y0 + i * step, c->width - 16, s, now_ms);
    }
}

// The mode's name, outlined in its color: SPORT the accent, DRIFT and
// TRACK amber, RACE (everything off) red.
static int draw_badge(canvas_t *c, const theme_t *t, int cy, drive_mode_t mode) {
    static const char *const names[DRIVE_MODES] = {
        [DRIVE_DRIFT] = "DRIFT MODE",
        [DRIVE_SPORT] = "SPORT MODE",
        [DRIVE_TRACK] = "TRACK MODE",
        [DRIVE_RACE] = "RACE MODE",
    };
    const char *name = names[mode] ? names[mode] : "";
    color_t color = mode == DRIVE_RACE ? t->danger : mode == DRIVE_SPORT ? t->accent : t->warning;
    int w = gfx_text_width(&font_label, name, 1) + 20, h = 22;
    stroke_round_rect(c, (c->width - w) / 2, cy - h / 2, w, h, h / 2, 2, color, t->background);
    draw_text_centered(c, &font_label, c->width / 2, cy + font_label.cap / 2, name, color, 1);
    return h;
}

void draw_track_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, drive_mode_t mode,
                     uint32_t now_ms) {
    const tile_t *oil = tile_for(SIG_OIL), *afr = tile_for(SIG_AFR);
    char max[24], lean[24];
    max_line(oil, "MAX", s, max);
    max_line(afr, "LEAN", s, lean);
    char gear[4];
    gear_text(s, now_ms, gear);
    if (is_landscape(c)) {
        int size = 125, y = top + 2;
        draw_dial(c, t, oil, 0, y, size, &track_style, s, max, false, now_ms);
        draw_dial(c, t, afr, c->width - size, y, size, &track_style, s, lean, false, now_ms);
        draw_badge(c, t, top + 14, mode);
        draw_text_centered(c, &font_huge, c->width / 2, y + 30 + font_huge.cap, gear, t->label, 0);
        int rows_y = (y + size + bottom - ROW_HEIGHT) / 2, w = c->width / 2 - 14;
        draw_row(c, t, tile_for(SIG_COOLANT), 4, rows_y, w, s, now_ms);
        draw_row(c, t, tile_for(SIG_IAT), c->width - w - 4, rows_y, w, s, now_ms);
        return;
    }
    int h = draw_badge(c, t, top + 14, mode);
    int size = c->width / 2 - 2, y = top + h + 6;
    draw_dial(c, t, oil, 0, y, size, &track_portrait_style, s, max, false, now_ms);
    draw_dial(c, t, afr, c->width - size, y, size, &track_portrait_style, s, lean, false, now_ms);
    int rows_y = bottom - 2 * ROW_HEIGHT - 3 - 6;
    draw_text_centered(c, &font_huge, c->width / 2, (y + size + rows_y + font_huge.cap) / 2, gear, t->label, 0);
    draw_row(c, t, tile_for(SIG_COOLANT), 8, rows_y, c->width - 16, s, now_ms);
    draw_row(c, t, tile_for(SIG_IAT), 8, rows_y + ROW_HEIGHT + 3, c->width - 16, s, now_ms);
}
