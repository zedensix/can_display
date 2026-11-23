#pragma once

// The data source: the MCP2515 on the CAN bus, or the sim build's fake
// data (SIM=1); Send log over Bluetooth LE beside it; and the reboots and
// crash marks around them.

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"
#include "pico/time.h"
#include "ui/screen.h"

#ifndef SIM
#define SIM 0
#endif

static inline uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

// boot_t is why the previous run ended, from the watchdog's scratch
// registers, which survive a reboot (not a power cut).
typedef struct {
    bool froze, hard_fault;
} boot_t;

// boot_reason reads why the previous run ended, and clears it.
boot_t boot_reason(void);

void source_start(state_t *state);
// poll_data_source runs the CAN source (paused during Send log) and the radio.
void poll_data_source(uint32_t now);
// give_click_to_prompt hands a click on the gauges page to what's waiting
// for one (ending Send log, the MCP2515's Retry), and reports whether
// something took it.
bool give_click_to_prompt(void);
// run_settings_action carries out a click on an action row of the settings page.
void run_settings_action(action_t action);
void __attribute__((noreturn)) reboot_board(void);
