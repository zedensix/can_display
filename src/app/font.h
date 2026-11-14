#pragma once

#include <stdint.h>

// glyph_t and font_t follow the Adafruit GFX font format.
typedef struct {
    uint16_t bitmap_offset;
    uint8_t width, height, x_advance;
    int8_t x_offset, y_offset;
} glyph_t;

typedef struct {
    const uint8_t *bitmap;
    const glyph_t *glyphs;
    uint8_t first, last, y_advance;
} font_t;

extern const font_t font_bold9, font_bold12, font_bold18, font_bold24;
