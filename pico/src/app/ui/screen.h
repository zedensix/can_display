#pragma once

#include <stdbool.h>

#include <stdint.h>

#include "data/signals.h"
#include "store/settings.h"

// screen_t is what the rotary encoder drives (the simulator's keyboard
// stands in for it).
//
// On the gauges, turning cycles the views: every gauge, then each one
// alone. A click opens the settings. There, turning moves the cursor, a
// click toggles the row under it or, on brightness and the shift light,
// makes turning change the value until the next click. Send log returns
// to the gauges and sets action. Back returns to the gauges.
//
// While the data source waits on the driver (state_t.wait: the MCP2515's
// self test failed), turning on the gauges picks between Retry and
// Settings instead, and a click does it. On the door screen, a click
// dismisses it until another door opens. Nothing coming in (a status with
// nothing to do but wait, or Retry) and no input for SCREEN_IDLE_MS, the
// screensaver comes on; the next input only wakes it, a status changing
// (an update from the phone) too.

typedef enum {
    PAGE_GAUGES,
    PAGE_SETTINGS,
} page_t;

// Rows of the settings page, top to bottom, in its groups: display,
// tools, then Back.
enum {
    ROW_BRIGHTNESS,
    ROW_SHIFT_LIGHT,
    ROW_FLIP,
    ROW_SEND_LOG,
    ROW_BACK,
    SETTINGS_ROWS,
};

// action_t is a click on an action row, for the caller to act on and
// clear.
typedef enum {
    ACTION_NONE,
    ACTION_SEND_LOG,
} action_t;

// choice_t is what a click does while the data source waits on the
// driver: CHOICE_RETRY is left to the caller, CHOICE_SETTINGS opens the
// settings.
typedef enum { CHOICE_RETRY, CHOICE_SETTINGS } choice_t;

#define SCREEN_IDLE_MS 30000

typedef struct {
    page_t page;
    // view is 0 for every gauge, k for the k-th gauge alone.
    int view;
    // cursor is the selected settings row.
    int cursor;
    // editing makes turning change the value of the row under the cursor.
    bool editing;
    action_t action;
    choice_t choice;
    // input_ms is the last input, or the status changing (status, as last
    // seen): an update's progress keeps the screen on; idle, the
    // screensaver on.
    uint32_t input_ms;
    bool idle;
    char status[sizeof((state_t *)0)->status];
    // doors_dismissed: the open doors (DOOR_* bits) whose screen a click
    // put away; a door shut leaves it, so opening it again shows it.
    uint8_t doors_dismissed;
} screen_t;

// screen_apply_input applies the knob's input at now (none: 0 and false,
// every loop, for the screensaver to come on). It returns whether a click
// is left for the caller: the data source's prompt first, else
// screen_apply_click.
bool screen_apply_input(screen_t *s, const state_t *state, int detents, bool press, int views, settings_t *cfg,
                        uint32_t now);

// screen_shows_doors is true when the gauges page gives way to the door
// screen: a door open that wasn't dismissed.
bool screen_shows_doors(const screen_t *s, const state_t *state, uint32_t now);

// screen_apply_turn applies encoder detents, positive clockwise. views is how
// many gauges the single-gauge views cycle through.
void screen_apply_turn(screen_t *s, int detents, int views, settings_t *cfg);

// screen_apply_click applies a click.
void screen_apply_click(screen_t *s, settings_t *cfg);
