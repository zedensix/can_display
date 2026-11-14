#pragma once

#include <stdbool.h>
#include <stdint.h>

// The KY-040 rotary encoder: rotation decoded on GPIO interrupts, a
// detent counted when it comes to rest; the push button sampled every
// loop, debounced, acting as it goes down.
void knob_init(void);

// knob_poll returns the detents turned since the last call (positive
// clockwise; swap CLK and DT if it's backwards), whether the button just
// went down, and whether it has just been held for KNOB_LONG_PRESS_MS.
#define KNOB_LONG_PRESS_MS 1000
int knob_poll(uint32_t now_ms, bool *press, bool *long_press);

// knob_pending is true when knob_poll has something new: the main loop
// cuts its wait short to draw it at once.
bool knob_pending(void);
