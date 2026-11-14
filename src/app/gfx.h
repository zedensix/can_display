#pragma once

#include <stdint.h>

#include "font.h"

// color_t is RGB565, the panel's pixel format.
typedef uint16_t color_t;

#define RGB(r, g, b) ((color_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// canvas_t is a band of a width x height screen held in RAM: rows
// band_top to band_top + band_height - 1. Drawing outside the band is
// dropped, so a frame can be drawn band by band in less RAM.
typedef struct {
    color_t *pixels;
    int width, height;
    int band_top, band_height;
} canvas_t;

// DEGREE is the degree sign in strings drawn by gfx_text. The fonts
// don't have it, so gfx_text draws a ring instead.
#define DEGREE "\xb0"

void gfx_fill_rect(canvas_t *c, int x, int y, int w, int h, color_t color);

// gfx_text draws s with its baseline at y, each font pixel as a scale x
// scale square, and returns the x after the last character.
int gfx_text(canvas_t *c, const font_t *font, int x, int y, const char *s, color_t color, int scale);

// gfx_text_width is the width gfx_text advances by at scale 1.
int gfx_text_width(const font_t *font, const char *s);

// gfx_blend mixes from into to: 0 gives from, 1 gives to.
color_t gfx_blend(color_t from, color_t to, float amount);
