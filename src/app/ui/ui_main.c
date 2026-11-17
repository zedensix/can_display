// The gauges page's every-gauge view: the oil temperature first, big,
// with its session's highest and a long segment bar (on a FA20, the oil
// is what a track day kills), then the other tiles as a list, a row each:
// label, a short bar, the value. Portrait: the oil across the top, the
// list under it; landscape: the oil on the left, the list on its right.
// A reading in danger blinks its whole block or row red, as a tile does.

#include <stdio.h>

#include "ui_internal.h"

#define HERO_SEGMENTS 20
#define ROW_SEGMENTS 10
#define ROW_LABEL_W 92
#define ROW_VALUE_W 78
// A row's bar only where it gets at least this wide.
#define ROW_BAR_MIN 30

// draw_hero draws tile big in the box x, y, w, h.
static void draw_hero(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                      uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    color_t ink = alarm ? t->alarm_text : t->label;
    gfx_text(c, &font_bold12, x + 8, y + 18, tile->label, ink, 1);
    const reading_t *r = &s->readings[tile->signal];
    if (r->valid) {
        // The session's highest beside the label, else over the bar.
        char number[16], high[32];
        snprintf(number, sizeof number, tile->format, (double)r->max);
        snprintf(high, sizeof high, "MAX %s%s", number, tile->unit);
        int high_w = gfx_text_width(label_font, high);
        bool beside = gfx_text_width(&font_bold12, tile->label) + 8 + high_w <= w - 16;
        gfx_text(c, label_font, beside ? x + w - 8 - high_w : x + 8, beside ? y + 18 : y + h - 26, high,
                 alarm ? t->alarm_text : gfx_blend(t->background, t->label, 0.6f), 1);
    }
    // Twice the big font where it fits, the unit dropped first (the label
    // says what it is), else once.
    int scale = 2;
    if (gfx_text_width(&font_bold24, text) * 2 > w - 16) {
        char number[16];
        snprintf(number, sizeof number, tile->format, (double)r->value);
        if (fraction >= 0 && gfx_text_width(&font_bold24, number) * 2 <= w - 16) {
            snprintf(text, 24, "%s", number);
        } else {
            scale = 1;
        }
    }
    // The 24pt digits rise about 26px above their baseline.
    draw_text_centered(c, &font_bold24, x + w / 2, y + h / 2 + 13 * scale + 6, text, alarm ? t->alarm_text : color,
                       scale);
    color_t colors[HERO_SEGMENTS];
    gauge_segment_colors(&tile->range, fraction, -1, HERO_SEGMENTS, alarm, t, colors);
    draw_segment_bar(c, x + 8, y + h - 18, w - 16, 10, HERO_SEGMENTS, colors);
}

// draw_row draws tile as a row of the list in the box x, y, w, h.
static void draw_row(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                     uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    int baseline = y + h / 2 + 5;
    gfx_text(c, label_font, x + 8, baseline, tile->label, alarm ? t->alarm_text : t->label, 1);
    const font_t *font = gfx_text_width(&font_bold18, text) <= ROW_VALUE_W ? &font_bold18 : &font_bold12;
    gfx_text(c, font, x + w - 8 - gfx_text_width(font, text), y + h / 2 + 8, text, alarm ? t->alarm_text : color, 1);
    int bar_x = x + 8 + ROW_LABEL_W, bar_w = w - 16 - ROW_LABEL_W - ROW_VALUE_W - 8;
    if (bar_w >= ROW_BAR_MIN) {
        color_t colors[ROW_SEGMENTS];
        gauge_segment_colors(&tile->range, fraction, -1, ROW_SEGMENTS, alarm, t, colors);
        draw_segment_bar(c, bar_x, y + h / 2 - 3, bar_w, 6, ROW_SEGMENTS, colors);
    }
    gfx_fill_rect(c, x + 4, y + h - 1, w - 8, 1, gfx_blend(t->background, t->frame, 0.6f));
}

void draw_main_layout(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, uint32_t now_ms) {
    // ui_tiles' first is the oil; the rest of the grid's tiles, the list.
    int rows = ui_tile_count - 1;
    if (is_landscape(c)) {
        int hero_w = c->width * 2 / 5;
        draw_hero(c, t, &ui_tiles[0], 0, top, hero_w, bottom - top, s, now_ms);
        gfx_fill_rect(c, hero_w, top + 4, 1, bottom - top - 8, t->frame);
        int row_h = (bottom - top) / rows;
        for (int i = 0; i < rows; i++) {
            draw_row(c, t, &ui_tiles[1 + i], hero_w + 1, top + i * row_h, c->width - hero_w - 1, row_h, s, now_ms);
        }
        return;
    }
    int hero_h = (bottom - top) * 9 / 20;
    draw_hero(c, t, &ui_tiles[0], 0, top, c->width, hero_h, s, now_ms);
    gfx_fill_rect(c, 4, top + hero_h, c->width - 8, 1, t->frame);
    int row_h = (bottom - top - hero_h - 1) / rows;
    for (int i = 0; i < rows; i++) {
        draw_row(c, t, &ui_tiles[1 + i], 0, top + hero_h + 1 + i * row_h, c->width, row_h, s, now_ms);
    }
}
