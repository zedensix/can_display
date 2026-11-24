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

// DEGREE is the degree sign in strings drawn by gfx_text: the fonts hold
// it at 127.
#define DEGREE "\xb0"

void gfx_fill_rect(canvas_t *c, int x, int y, int w, int h, color_t color);

// gfx_text draws s from x with its baseline at y, anti-aliased over what's
// there, spacing more pixels between its letters, and returns the x after
// the last one.
int gfx_text(canvas_t *c, const font_t *font, int x, int y, const char *s, color_t color, int spacing);

// gfx_text_width is the width gfx_text advances by.
int gfx_text_width(const font_t *font, const char *s, int spacing);

// gfx_fill_polygon fills the convex polygon of n corners (xs, ys, in
// pixels, fractions included), anti-aliased over what's there: a dial's
// segments, its glow, a tick.
void gfx_fill_polygon(canvas_t *c, const float xs[], const float ys[], int n, color_t color);

// gfx_blend mixes from into to: 0 gives from, 1 gives to.
color_t gfx_blend(color_t from, color_t to, float amount);

// gfx_mix is gfx_blend in integers, amount out of 255: per pixel.
color_t gfx_mix(color_t from, color_t to, unsigned amount);
