#include "gfx.h"

#include <math.h>
#include <string.h>

void gfx_fill_rect(canvas_t *c, int x, int y, int w, int h, color_t color) {
    int x0 = x < 0 ? 0 : x;
    int x1 = x + w > c->width ? c->width : x + w;
    int y0 = y < c->band_top ? c->band_top : y;
    int band_end = c->band_top + c->band_height;
    int y1 = y + h > band_end ? band_end : y + h;
    for (int row = y0; row < y1; row++) {
        color_t *p = c->pixels + (row - c->band_top) * c->width;
        for (int col = x0; col < x1; col++) {
            p[col] = color;
        }
    }
}

// Shifts, not divisions: per pixel, on a chip without a divide in its
// core (255 out of 255 rounds up to 256 out of 256, the whole color).
color_t gfx_mix(color_t from, color_t to, unsigned amount) {
    unsigned a = amount + (amount >> 7), na = 256 - a;
    unsigned r = ((from >> 11) * na + (to >> 11) * a) >> 8;
    unsigned g = (((from >> 5) & 63) * na + ((to >> 5) & 63) * a) >> 8;
    unsigned b = ((from & 31) * na + (to & 31) * a) >> 8;
    return (color_t)(r << 11 | g << 5 | b);
}

color_t gfx_blend(color_t from, color_t to, float amount) {
    return gfx_mix(from, to, amount <= 0 ? 0 : amount >= 1 ? 255 : (unsigned)(amount * 255));
}

// blend_px blends color into the pixel at x, y by amount (out of 255).
static void blend_px(canvas_t *c, int x, int y, color_t color, unsigned amount) {
    if (x < 0 || x >= c->width || y < c->band_top || y >= c->band_top + c->band_height || amount == 0) {
        return;
    }
    color_t *p = c->pixels + (y - c->band_top) * c->width + x;
    *p = amount >= 255 ? color : gfx_mix(*p, color, amount);
}

static const glyph_t *glyph_for(const font_t *font, unsigned char ch) {
    if (ch == 0xb0) {
        ch = 127;
    }
    if (ch < font->first || ch > font->last) {
        return 0;
    }
    return &font->glyphs[ch - font->first];
}

int gfx_text(canvas_t *c, const font_t *font, int x, int y, const char *s, color_t color, int spacing) {
    for (; *s; s++) {
        const glyph_t *g = glyph_for(font, (unsigned char)*s);
        if (!g) {
            continue;
        }
        const uint8_t *px = font->bitmap + g->bitmap_offset;
        int top = y + g->y_offset, left = x + g->x_offset;
        // Rows outside the band skipped whole: most of a glyph, drawing in bands.
        for (int row = 0; row < g->height; row++) {
            if (top + row < c->band_top || top + row >= c->band_top + c->band_height) {
                continue;
            }
            for (int col = 0; col < g->width; col++) {
                int i = row * g->width + col;
                unsigned shade = i % 2 ? px[i / 2] & 15 : px[i / 2] >> 4;
                blend_px(c, left + col, top + row, color, shade * 17);
            }
        }
        x += g->x_advance + spacing;
    }
    return x;
}

int gfx_text_width(const font_t *font, const char *s, int spacing) {
    int width = 0, n = 0;
    for (; *s; s++) {
        const glyph_t *g = glyph_for(font, (unsigned char)*s);
        if (g) {
            width += g->x_advance;
            n++;
        }
    }
    return width + (n > 1 ? (n - 1) * spacing : 0);
}

// Sub-scanlines per pixel row: the vertical anti-aliasing (each span's
// ends are exact horizontally).
#define SUBROWS 4
#define MAX_WIDTH 320
#define MAX_CORNERS 8

// In fixed point, 8 bits of fraction: the Pico has no floating point
// unit, and a dial is a few hundred of these a frame.
#define FIX(v) ((int32_t)lroundf((v) * 256))

void gfx_fill_polygon(canvas_t *c, const float xs[], const float ys[], int n, color_t color) {
    if (n > MAX_CORNERS) {
        return;
    }
    int32_t px[MAX_CORNERS], py[MAX_CORNERS];
    int64_t slope[MAX_CORNERS];
    int32_t min_x = INT32_MAX, max_x = INT32_MIN, min_y = INT32_MAX, max_y = INT32_MIN;
    for (int i = 0; i < n; i++) {
        px[i] = FIX(xs[i]);
        py[i] = FIX(ys[i]);
        min_x = px[i] < min_x ? px[i] : min_x;
        max_x = px[i] > max_x ? px[i] : max_x;
        min_y = py[i] < min_y ? py[i] : min_y;
        max_y = py[i] > max_y ? py[i] : max_y;
    }
    // Each edge's x per y, 16 bits of fraction: one division an edge.
    for (int i = 0; i < n; i++) {
        int32_t dy = py[(i + 1) % n] - py[i];
        slope[i] = dy ? (int64_t)(px[(i + 1) % n] - px[i]) * 65536 / dy : 0;
    }
    int x0 = min_x >> 8, x1 = (max_x + 255) >> 8, y0 = min_y >> 8, y1 = (max_y + 255) >> 8;
    x0 = x0 < 0 ? 0 : x0;
    x1 = x1 > c->width ? c->width : x1;
    y0 = y0 < c->band_top ? c->band_top : y0;
    y1 = y1 > c->band_top + c->band_height ? c->band_top + c->band_height : y1;
    if (x1 <= x0 || x1 - x0 > MAX_WIDTH) {
        return;
    }
    // A row's coverage per pixel, 64 per full sub-row.
    uint16_t cover[MAX_WIDTH];
    for (int y = y0; y < y1; y++) {
        memset(cover, 0, sizeof cover[0] * (size_t)(x1 - x0));
        for (int s = 0; s < SUBROWS; s++) {
            int32_t sy = (y << 8) + s * (256 / SUBROWS) + 256 / SUBROWS / 2;
            int32_t left = INT32_MAX, right = INT32_MIN;
            for (int i = 0; i < n; i++) {
                int32_t ay = py[i], by = py[(i + 1) % n];
                if ((ay <= sy && by > sy) || (by <= sy && ay > sy)) {
                    // >> rounds towards minus infinity on a negative: no division per sub-row.
                    int32_t x = px[i] + (int32_t)(((int64_t)(sy - ay) * slope[i]) >> 16);
                    left = x < left ? x : left;
                    right = x > right ? x : right;
                }
            }
            left = left < (x0 << 8) ? x0 << 8 : left;
            right = right > (x1 << 8) ? x1 << 8 : right;
            if (right <= left) {
                continue;
            }
            int il = left >> 8, ir = right >> 8;
            if (il == ir) {
                cover[il - x0] += (uint16_t)((right - left) >> 2);
                continue;
            }
            cover[il - x0] += (uint16_t)((256 - (left & 255)) >> 2);
            for (int x = il + 1; x < ir; x++) {
                cover[x - x0] += 64;
            }
            if (ir < x1) {
                cover[ir - x0] += (uint16_t)((right & 255) >> 2);
            }
        }
        for (int x = x0; x < x1; x++) {
            unsigned amount = cover[x - x0];
            blend_px(c, x, y, color, amount > 255 ? 255 : amount);
        }
    }
}
