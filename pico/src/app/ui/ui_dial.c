// A gauge as a round dial, the Android app's (Pages.kt dial): segments
// over 270 degrees like the tachometer's, each in its band's color, a
// glow under the lit ones, the session's highest (and lowest) ticked
// across the ring; inside, the label in the accent, the value with its
// unit up beside it, a line under. And one gauge alone: a big dial, the
// session's lowest and highest, what its colors mean.

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui_internal.h"

#define DIAL_START 135.0f
#define DIAL_SWEEP 270.0f

// tick marks v across the ring, out past both its edges.
static void tick(canvas_t *c, float cx, float cy, float r_in, float r_out, float fraction, color_t color) {
    float a = (DIAL_START + DIAL_SWEEP * fraction) * PI / 180, half = 0.9f * PI / 180;
    float r0 = r_in - 3, r1 = r_out + 3;
    float xs[] = {cx + r0 * cosf(a - half), cx + r1 * cosf(a - half), cx + r1 * cosf(a + half),
                  cx + r0 * cosf(a + half)};
    float ys[] = {cy + r0 * sinf(a - half), cy + r1 * sinf(a - half), cy + r1 * sinf(a + half),
                  cy + r0 * sinf(a + half)};
    gfx_fill_polygon(c, xs, ys, 4, color);
}

// The label, the value with its unit, extra: centered as a block on cx, cy
// inside the ring's inner edge, r_in from it: the value in style's font
// or the largest smaller one that fits, extra left out if it doesn't.
static void dial_text(canvas_t *c, const theme_t *t, const dial_style_t *style, int cx, int cy, float r_in,
                      const char *label, const char *value, const char *unit, color_t color, const char *extra) {
    static const font_t *const smaller[] = {&font_big, &font_large, &font_value, &font_text};
    const font_t *font = style->value;
    int wu = unit[0] ? gfx_text_width(style->unit, unit, 0) + 2 : 0;
    int room = (int)(r_in * 1.7f);
    for (int i = 0; i < 4; i++) {
        if (smaller[i]->cap > font->cap) {
            continue;
        }
        font = smaller[i];
        int height = style->label->cap + font->cap * 5 / 4;
        if (gfx_text_width(font, value, 0) + wu <= room && height <= room * 3 / 4) {
            break;
        }
    }
    int gap1 = font->cap / 4, gap2 = font->cap / 5;
    // The line under it fits in the ring's opening, narrower lower down.
    bool has_extra = style->extra && extra[0] && gfx_text_width(style->extra, extra, 1) <= room * 3 / 4;
    int height = style->label->cap + gap1 + font->cap + (has_extra ? gap2 + style->extra->cap : 0);
    int y = cy - height / 2 + style->label->cap;
    draw_text_centered(c, style->label, cx, y, label, t->accent, 2);
    y += gap1 + font->cap;
    int wv = gfx_text_width(font, value, 0);
    int left = cx - (wv + wu) / 2;
    gfx_text(c, font, left, y, value, color, 0);
    if (unit[0]) {
        gfx_text(c, style->unit, left + wv + 2, y - font->cap + style->unit->cap, unit, t->label, 0);
    }
    if (has_extra) {
        draw_text_centered(c, style->extra, cx, y + gap2 + style->extra->cap, extra, t->dim, 1);
    }
}

void draw_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int size, const dial_style_t *style,
               const state_t *s, const char *extra, bool low, uint32_t now_ms) {
    shown_t shown = tile_shown(tile, s, now_ms, t);
    const reading_t *r = &s->readings[tile->signal];
    float cx = (float)x + (float)size / 2, cy = (float)y + (float)size / 2;
    float mid = (float)size / 2 - (float)style->ring / 2 - 5;
    float r_in = mid - (float)style->ring / 2, r_out = mid + (float)style->ring / 2;
    if (shown.fraction >= 0) {
        // The glow: a wider arc, faint, under the lit segments.
        float sweep = fmaxf(1, DIAL_SWEEP * shown.fraction);
        color_t glow = gfx_blend(t->background, band_color(&tile->range, r->value, t), 0.16f);
        fill_arc(c, cx, cy, r_in - 4, r_out + 4, DIAL_START, DIAL_START + sweep, glow);
    }
    color_t colors[48];
    int n = style->segments > 48 ? 48 : style->segments;
    gauge_segment_colors(&tile->range, shown.fraction, n, t, colors);
    float step = DIAL_SWEEP / (float)n, gap = step * 0.13f;
    for (int i = 0; i < n; i++) {
        float a0 = DIAL_START + (float)i * step + gap;
        fill_arc(c, cx, cy, r_in, r_out, a0, a0 + step - 2 * gap, colors[i]);
    }
    if (r->valid) {
        tick(c, cx, cy, r_in, r_out, scale_fraction(&tile->range, r->max), t->label);
        if (low) {
            tick(c, cx, cy, r_in, r_out, scale_fraction(&tile->range, r->min), t->cold);
        }
    }
    bool blink = shown.danger && alarm_blink_lit(now_ms);
    const char *label = low ? tile->label : tile->short_label;
    dial_text(c, t, style, (int)cx, (int)cy, r_in, label, shown.number, shown.fraction >= 0 ? tile->unit : "",
              blink ? t->label : shown.color, extra);
}

void draw_scale_ends(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int size) {
    int inset = size * 26 / 340, baseline = y + size - inset;
    char end[16];
    tile_number(tile, tile->range.min, end);
    end[strcspn(end, ".")] = 0;
    gfx_text(c, &font_small, x + inset * 23 / 10, baseline, end, t->dim, 0);
    tile_number(tile, tile->range.max, end);
    end[strcspn(end, ".")] = 0;
    draw_text_right(c, &font_small, x + size - inset * 21 / 10, baseline, end, t->dim, 0);
}

// The legend's height: MIN and MAX, a line, four bands.
#define LEGEND_H 140

static const dial_style_t alone_style = {12, 44, &font_label, &font_big, &font_text, NULL};

// The bands from the bottom of the scale up, each with the value it
// starts at: what the gauge's colors mean.
static int bands(const range_t *r, const theme_t *t, float from[], color_t colors[]) {
    float edges[] = {r->min, r->danger_low, r->warn_low, r->cold, r->warn_high, r->danger_high};
    float sorted[6];
    int n = 0;
    for (int i = 0; i < 6; i++) {
        if (i == 0 || (edges[i] > r->min && edges[i] < r->max)) {
            int at = n++;
            while (at > 0 && sorted[at - 1] > edges[i]) {
                sorted[at] = sorted[at - 1];
                at--;
            }
            sorted[at] = edges[i];
        }
    }
    int count = 0;
    for (int i = 0; i < n; i++) {
        // Just past the edge: band_color counts the edge itself in the band under it.
        color_t color = band_color(r, sorted[i] + (r->max - r->min) * 0.001f, t);
        if (count && (colors[count - 1] == color || from[count - 1] == sorted[i])) {
            continue;
        }
        from[count] = sorted[i];
        colors[count++] = color;
    }
    return count;
}

// The session's lowest and highest side by side, column apart, then
// where each color starts, from x, top down: LEGEND_H high.
static void legend(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int top, int column, int right,
                   const reading_t *r) {
    const char *unit = strcmp(tile->unit, DEGREE "C") == 0 ? DEGREE : tile->unit;
    for (int i = 0; i < 2; i++) {
        int left = x + i * column;
        gfx_text(c, &font_small, left, top + font_small.cap, i ? "MAX" : "MIN", t->dim, 1);
        char value[24] = "--";
        color_t color = t->dim;
        if (r->valid) {
            char number[16];
            tile_number(tile, i ? r->max : r->min, number);
            snprintf(value, sizeof value, "%s%s", number, unit);
            color = i ? band_color(&tile->range, r->max, t) : t->cold;
        }
        gfx_text(c, &font_value, left, top + font_small.cap + 5 + font_value.cap, value, color, 0);
    }
    int line = top + font_small.cap + 5 + font_value.cap + 7;
    gfx_fill_rect(c, x, line, right - x, 1, t->frame);
    float from[6];
    color_t colors[6];
    int n = bands(&tile->range, t, from, colors);
    for (int i = 0; i < n; i++) {
        int baseline = line + 8 + font_small.cap + i * (font_small.cap + 7);
        char a[16], b[16], span[40];
        tile_number(tile, from[i], a);
        if (i + 1 < n) {
            tile_number(tile, from[i + 1], b);
        }
        if (n == 1) {
            snprintf(span, sizeof span, "all");
        } else if (i == 0) {
            snprintf(span, sizeof span, "< %s", b);
        } else if (i + 1 == n) {
            snprintf(span, sizeof span, "%s+", a);
        } else {
            snprintf(span, sizeof span, "%s-%s", a, b);
        }
        gfx_fill_rect(c, x, baseline - font_small.cap / 2 - 3, 14, 6, colors[i]);
        gfx_text(c, &font_small, x + 20, baseline, span, t->label, 0);
    }
}

void draw_alone_page(canvas_t *c, const theme_t *t, const tile_t *tile, int top, int bottom, const state_t *s,
                     uint32_t now_ms) {
    const reading_t *r = &s->readings[tile->signal];
    int avail = bottom - top;
    if (is_landscape(c)) {
        int size = avail - 4 < 196 ? avail - 4 : 196;
        int y = top + (avail - size) / 2;
        draw_dial(c, t, tile, 0, y, size, &alone_style, s, "", true, now_ms);
        draw_scale_ends(c, t, tile, 0, y, size);
        legend(c, t, tile, size + 10, top + (avail - LEGEND_H) / 2, 58, c->width - 4, r);
        return;
    }
    int size = avail - LEGEND_H - 8 < c->width - 20 ? avail - LEGEND_H - 8 : c->width - 20;
    int x = (c->width - size) / 2;
    draw_dial(c, t, tile, x, top + 2, size, &alone_style, s, "", true, now_ms);
    draw_scale_ends(c, t, tile, x, top + 2, size);
    legend(c, t, tile, 20, top + size + 6, 110, c->width - 20, r);
}
