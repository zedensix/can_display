#include "gfx.h"

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

static int degree_radius(const font_t *font) {
    int r = font->y_advance / 10;
    return r < 2 ? 2 : r;
}

// draw_degree draws a ring at the top of the line, standing for the
// degree sign, and returns its advance.
static int draw_degree(canvas_t *c, const font_t *font, int x, int y, color_t color, int scale) {
    int r = degree_radius(font);
    int inner = r - (r / 2 > 1 ? r / 2 : 1);
    int cx = x + r + 1, cy = y - font->y_advance * 6 / 10 + r;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int d = dx * dx + dy * dy;
            if (d <= r * r && d > inner * inner) {
                gfx_fill_rect(c, x + (cx - x + dx) * scale, y + (cy - y + dy) * scale, scale, scale, color);
            }
        }
    }
    return 2 * r + 3;
}

int gfx_text(canvas_t *c, const font_t *font, int x, int y, const char *s, color_t color, int scale) {
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == 0xb0) {
            x += draw_degree(c, font, x, y, color, scale) * scale;
            continue;
        }
        if (ch < font->first || ch > font->last) {
            continue;
        }
        const glyph_t *g = &font->glyphs[ch - font->first];
        const uint8_t *bits = font->bitmap + g->bitmap_offset;
        int bit = 0;
        for (int row = 0; row < g->height; row++) {
            for (int col = 0; col < g->width; col++, bit++) {
                if (bits[bit / 8] & (0x80 >> (bit % 8))) {
                    gfx_fill_rect(c, x + (g->x_offset + col) * scale, y + (g->y_offset + row) * scale, scale, scale,
                                  color);
                }
            }
        }
        x += g->x_advance * scale;
    }
    return x;
}

int gfx_text_width(const font_t *font, const char *s) {
    int width = 0;
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == 0xb0) {
            width += 2 * degree_radius(font) + 3;
        } else if (ch >= font->first && ch <= font->last) {
            width += font->glyphs[ch - font->first].x_advance;
        }
    }
    return width;
}

color_t gfx_blend(color_t from, color_t to, float amount) {
    int r0 = from >> 11, g0 = (from >> 5) & 0x3F, b0 = from & 0x1F;
    int r1 = to >> 11, g1 = (to >> 5) & 0x3F, b1 = to & 0x1F;
    int r = r0 + (int)((float)(r1 - r0) * amount);
    int g = g0 + (int)((float)(g1 - g0) * amount);
    int b = b0 + (int)((float)(b1 - b0) * amount);
    return (color_t)((r << 11) | (g << 5) | b);
}
