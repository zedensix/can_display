// What a gauge shows, whatever draws it: its number, its band's color,
// where it sits on its scale, and its scale's segments.

#include <stdio.h>

#include "ui_internal.h"

color_t band_color(const range_t *r, float v, const theme_t *t) {
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

float scale_fraction(const range_t *r, float v) {
    if (r->max <= r->min) {
        return 0;
    }
    float f = (v - r->min) / (r->max - r->min);
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

void tile_number(const tile_t *tile, float v, char out[16]) {
    snprintf(out, 16, tile->format, (double)v);
}

// Off the throttle the AFR is no mixture to judge (the fuel is cut while
// decelerating, it reads lean): no band color, no alarm, white.
shown_t tile_shown(const tile_t *tile, const state_t *s, uint32_t now_ms, const theme_t *t) {
    shown_t shown = {"--", t->dim, -1, false};
    reading_t reading = s->readings[tile->signal];
    if (reading_is_stale(&reading, now_ms, tile->stale_ms)) {
        return shown;
    }
    tile_number(tile, reading.value, shown.number);
    bool unjudged = tile->signal == SIG_AFR && state_off_throttle(s, now_ms, FAST_STALE_MS);
    color_t band = band_color(&tile->range, reading.value, t);
    shown.color = unjudged ? t->label : band;
    shown.danger = !unjudged && band == t->danger;
    shown.fraction = scale_fraction(&tile->range, reading.value);
    return shown;
}

// Like a GT86 cluster: each segment in the band of the value it stands
// for, lit up to fraction, the rest a glimmer of it.
void gauge_segment_colors(const range_t *r, float fraction, int count, const theme_t *t, color_t out[]) {
    for (int i = 0; i < count; i++) {
        float from = r->min + (float)i / (float)count * (r->max - r->min);
        float mid = r->min + ((float)i + 0.5f) / (float)count * (r->max - r->min);
        color_t band = band_color(r, mid, t);
        bool lit = fraction >= 0 && r->min + fraction * (r->max - r->min) > from;
        out[i] = lit ? band : gfx_blend(t->background, band, 0.16f);
    }
}

const tile_t *tile_for(signal_t signal) {
    for (int i = 0; i < ui_view_count; i++) {
        if (ui_tiles[i].signal == signal) {
            return &ui_tiles[i];
        }
    }
    return &ui_tiles[0];
}
