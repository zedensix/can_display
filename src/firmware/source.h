#pragma once

// The data source this boot runs: the ELM327 BLE dongle, the MCP2515, or
// the sim build's fake data (SIM=1); and the reboots and crash marks
// around it.

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"
#include "pico/time.h"
#include "store/settings.h"
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
    bool froze, hard_fault, connect_now;
} boot_t;

// boot_reason reads why the previous run ended, and clears it.
boot_t boot_reason(void);

// source_start starts the data source cfg picks; connect: right away, without asking.
void source_start(const settings_t *cfg, state_t *state, bool connect);
// source_running is the data source this boot runs: changing it reboots.
source_t source_running(void);
void poll_data_source(uint32_t now);
bool in_standby(void);
// give_click_to_prompt hands a click to the data source's prompt, if one waits for it.
bool give_click_to_prompt(uint32_t now);
// remember_connected_dongle keeps the dongle just connected to in the settings, as
// the one the connect prompt offers first.
void remember_connected_dongle(settings_t *cfg);
// hold_retries holds the data source's automatic retry after a failure
// while hold: the driver's in the settings, maybe switching sources.
void hold_retries(bool hold);
// run_settings_action carries out a click on an action row of the settings page.
void run_settings_action(action_t action);
void __attribute__((noreturn)) reboot_board(bool connect);
