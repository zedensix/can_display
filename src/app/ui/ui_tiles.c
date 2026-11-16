#include <math.h>
#include <stdio.h>

#include "ui_internal.h"

// Tried largest first: wider readings ("-14.2V") drop a size or two to
// stay inside their tile.
static const font_t *const value_fonts[] = {&font_bold24, &font_bold18, &font_bold12};

static color_t band_color(const range_t *r, float v, const theme_t *t) {
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

static float scale_fraction(const range_t *r, float v) {
    if (r->max <= r->min) {
        return 0;
    }
    float f = (v - r->min) / (r->max - r->min);
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

// format_tile_value is what tile shows for reading: its text, color and gauge
// fraction, or "--" once stale (fraction -1: nothing lit). Off the
// throttle the AFR is no mixture to judge (the fuel is cut while
// decelerating, it reads lean): no band color, no alarm, white.
static void format_tile_value(const tile_t *tile, const state_t *s, uint32_t now_ms, const theme_t *t, char text[24],
                              color_t *color, float *fraction) {
    reading_t reading = s->readings[tile->signal];
    if (reading_is_stale(&reading, now_ms, tile->stale_ms)) {
        snprintf(text, 24, "--");
        *color = t->stale;
        *fraction = -1;
        return;
    }
    char number[16];
    snprintf(number, sizeof number, tile->format, (double)reading.value);
    snprintf(text, 24, "%s%s", number, tile->unit);
    bool unjudged = tile->signal == SIG_AFR && state_off_throttle(s, now_ms, FAST_STALE_MS);
    *color = unjudged ? t->label : band_color(&tile->range, reading.value, t);
    *fraction = scale_fraction(&tile->range, reading.value);
}

// gauge_segment_colors colors count gauge segments like a GT86 cluster: lit up
// to fraction, each in the band of the value it stands for, the unlit
// ones a glimmer of that band so the red zone shows at rest. On an alarm
// tile, lit segments take the alarm text color. peak (a fraction, or -1
// for none) marks the session's highest value in the alarm text color.
static void gauge_segment_colors(const range_t *r, float fraction, float peak, int count, bool alarm, const theme_t *t,
                                 color_t out[]) {
    color_t back = alarm ? t->alarm : t->background;
    int lit = fraction < 0 ? 0 : (int)(fraction * (float)count + 0.5f);
    if (fraction > 0 && lit == 0) {
        lit = 1;
    }
    for (int i = 0; i < count; i++) {
        color_t band = band_color(r, r->min + ((float)i + 0.5f) / (float)count * (r->max - r->min), t);
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

// draw_segment_arc draws count segments around a ring centered on cx, cy, from
// start degrees over sweep degrees (clockwise, 0 pointing right), out of
// 3px squares: the panel has no line drawing.
static void draw_segment_arc(canvas_t *c, int cx, int cy, int r_in, int r_out, float start, float sweep, int count,
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

#define BAR_SEGMENTS 12

static void draw_tile(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                      uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    draw_outline(c, x, y, w, h, color == t->danger ? t->danger : t->frame);
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
    gauge_segment_colors(&tile->range, fraction, -1, BAR_SEGMENTS, alarm, t, colors);
    draw_segment_bar(c, x + 8, y + h - 16, w - 16, 8, BAR_SEGMENTS, colors);
}

#define DIAL_SEGMENTS 30
#define DIAL_START 135
#define DIAL_SWEEP 270
#define DIAL_WIDTH 14

// draw_tile_dial draws a tile alone, as a round dial like the GT86's
// tachometer: segments around the value, blown up as much as fits
// inside, up to 3 times (chunky, but readable from the seat, without a
// bigger font taking flash).
void draw_tile_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                    uint32_t now_ms) {
    reading_t r = s->readings[tile->signal];
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    int cx = x + w / 2, r_out = w / 2 - 4, cy = y + r_out + 8, r_in = r_out - DIAL_WIDTH;
    color_t colors[DIAL_SEGMENTS];
    float peak = r.valid ? scale_fraction(&tile->range, r.max) : -1;
    gauge_segment_colors(&tile->range, fraction, peak, DIAL_SEGMENTS, alarm, t, colors);
    draw_segment_arc(c, cx, cy, r_in, r_out, DIAL_START, DIAL_SWEEP, DIAL_SEGMENTS, colors);

    int width = gfx_text_width(&font_bold24, text);
    int scale = 3;
    while (scale > 1 && width * scale > 2 * r_in - 24) {
        scale--;
    }
    // The 24pt digits rise about 26px above their baseline.
    const int cap_height = 26;
    draw_text_centered(c, &font_bold24, cx, cy + cap_height * scale / 2, text, alarm ? t->alarm_text : color, scale);

    // The scale's ends, under the ring's opening, and the label below.
    char end[16];
    int ends_y = cy + r_out * 7 / 10 + 22, ends_dx = r_out * 7 / 10;
    color_t dim = alarm ? t->alarm_text : t->label;
    snprintf(end, sizeof end, tile->format, (double)tile->range.min);
    draw_text_centered(c, label_font, cx - ends_dx, ends_y, end, dim, 1);
    snprintf(end, sizeof end, tile->format, (double)tile->range.max);
    draw_text_centered(c, label_font, cx + ends_dx, ends_y, end, dim, 1);
    // The session's extremes, to read back after a run.
    if (r.valid) {
        char low[16], high[16], line[48];
        snprintf(low, sizeof low, tile->format, (double)r.min);
        snprintf(high, sizeof high, tile->format, (double)r.max);
        snprintf(line, sizeof line, "MIN %s%s   MAX %s%s", low, tile->unit, high, tile->unit);
        draw_text_centered(c, label_font, cx, ends_y + 30, line, alarm ? t->alarm_text : t->label, 1);
    }
    draw_text_centered(c, &font_bold12, cx, y + h - 14, tile->label, dim, 1);
}

// draw_grid draws every tile, from top down.
void draw_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, uint32_t now_ms) {
    int rows = (ui_tile_count + GRID_COLUMNS - 1) / GRID_COLUMNS;
    int cell_w = c->width / GRID_COLUMNS, cell_h = (c->height - top) / rows;
    for (int i = 0; i < ui_tile_count; i++) {
        int col = i % GRID_COLUMNS, row = i / GRID_COLUMNS;
        draw_tile(c, t, &ui_tiles[i], col * cell_w + GRID_GAP, top + row * cell_h + GRID_GAP, cell_w - 2 * GRID_GAP,
                  cell_h - 2 * GRID_GAP, s, now_ms);
    }
}

#define BADGE_HEIGHT 20

static const tile_t *tile_for(signal_t signal) {
    for (int i = 0; i < ui_tile_count; i++) {
        if (ui_tiles[i].signal == signal) {
            return &ui_tiles[i];
        }
    }
    return &ui_tiles[0];
}

// draw_track_grid is the gauges with the stability control off, on track:
// what matters there big (the oil and the AFR across the screen, the
// coolant and the intake under them), the battery and the ethanol left
// out, and a badge saying which mode it's in: TRC OFF amber, VSC OFF red.
void draw_track_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, drive_mode_t mode, uint32_t now_ms) {
    const char *label = mode == DRIVE_VSC_OFF ? "VSC OFF" : "TRC OFF";
    color_t badge = mode == DRIVE_VSC_OFF ? t->danger : t->warning;
    int badge_w = gfx_text_width(label_font, label) + 28;
    fill_round_rect(c, (c->width - badge_w) / 2, top + 3, badge_w, BADGE_HEIGHT, BADGE_HEIGHT / 2, badge,
                    t->background);
    draw_text_centered(c, label_font, c->width / 2, top + 3 + BADGE_HEIGHT / 2 + 5, label,
                       mode == DRIVE_VSC_OFF ? t->alarm_text : t->background, 1);

    int y0 = top + BADGE_HEIGHT + 6, row_h = (c->height - y0) / 3, half = c->width / 2;
    draw_tile(c, t, tile_for(SIG_OIL), GRID_GAP, y0 + GRID_GAP, c->width - 2 * GRID_GAP, row_h - 2 * GRID_GAP, s,
              now_ms);
    draw_tile(c, t, tile_for(SIG_AFR), GRID_GAP, y0 + row_h + GRID_GAP, c->width - 2 * GRID_GAP, row_h - 2 * GRID_GAP,
              s, now_ms);
    draw_tile(c, t, tile_for(SIG_COOLANT), GRID_GAP, y0 + 2 * row_h + GRID_GAP, half - 2 * GRID_GAP,
              row_h - 2 * GRID_GAP, s, now_ms);
    draw_tile(c, t, tile_for(SIG_IAT), half + GRID_GAP, y0 + 2 * row_h + GRID_GAP, half - 2 * GRID_GAP,
              row_h - 2 * GRID_GAP, s, now_ms);
}
