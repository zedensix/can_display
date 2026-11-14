#pragma once

#include <stdbool.h>
#include <stdint.h>

// The KY-040 rotary encoder: rotation decoded on GPIO interrupts, the
// push button sampled once per frame (which debounces it).
void knob_init(void);

// knob_poll returns the detents turned since the last call (positive
// clockwise; swap CLK and DT if it's backwards), whether the button was
// just released after a short press, and whether it has just been held
// for KNOB_LONG_PRESS_MS.
#define KNOB_LONG_PRESS_MS 1000
int knob_poll(uint32_t now_ms, bool *press, bool *long_press);
