#pragma once

#include <stdbool.h>

#include "ui/gfx.h"

// The ST7789 panel on SPI0 at 62.5MHz, fed from RAM by DMA. If pixels come
// out garbled on long or loose wires, halve DISPLAY_BAUD.
void display_init(void);
void display_set_flip(bool flip);
void display_set_brightness(int percent);

// display_sleep turns the backlight off and puts the panel to sleep. Only
// a reboot wakes it.
void display_sleep(void);

// display_push_rows starts sending c's band to the panel and returns at once:
// keep the BLE stack running until display_push_in_progress is false, then call
// display_wait_for_push before drawing into the band again.
void display_push_rows(const canvas_t *c);
bool display_push_in_progress(void);
void display_wait_for_push(void);
