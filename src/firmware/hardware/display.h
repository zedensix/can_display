#pragma once

#include <stdbool.h>

#include "ui/gfx.h"

// The ST7789 panel on SPI0 at 62.5MHz, fed from RAM by DMA. If pixels come
// out garbled on long or loose wires, halve DISPLAY_BAUD.
void display_init(void);
// display_set_orientation turns the picture: upside down with flip, a
// quarter turn with landscape (PANEL_HEIGHT wide, PANEL_WIDTH high).
void display_set_orientation(bool flip, bool landscape);
void display_set_brightness(int percent);

// display_push_rows starts sending c's band to the panel and returns at once:
// keep the data source running until display_push_in_progress is false, then call
// display_wait_for_push before drawing into the band again.
void display_push_rows(const canvas_t *c);
bool display_push_in_progress(void);
void display_wait_for_push(void);
