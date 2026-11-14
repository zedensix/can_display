#pragma once

#include <stdbool.h>

#include "gfx.h"

// The ST7789 panel on SPI0 at 62.5MHz, fed from RAM by DMA. If pixels come
// out garbled on long or loose wires, halve DISPLAY_BAUD.
void display_init(void);
void display_set_flip(bool flip);
void display_set_brightness(int percent);

// display_push starts sending c's band to the panel and returns at once:
// keep the BLE stack running until display_busy is false, then call
// display_finish before drawing into the band again.
void display_push(const canvas_t *c);
bool display_busy(void);
void display_finish(void);
