#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"
#include "store/datalog.h"

// The dash as a Bluetooth LE peripheral, "GT86 DASH" (dash.gatt), for Send
// log: the page in web/ on a phone writes 'S', gets a 4-byte size then the
// data log from its oldest record as notifications, and writes 'D' once it
// has it all. The radio is on only meanwhile. It runs from ble_poll_radio,
// in the main loop, along with the BTstack run loop (the cyw43 "poll"
// architecture: no interrupts racing the main loop).
//
// BTstack writes flash (its keys, in its own sectors) through the SDK's
// flash_safe_execute, which doesn't hold core1: core1, reading the CAN
// bus from flash, is held in RAM until Send log is over
// (store_hold_core1), and the CAN source paused, the gauges with it.

// ble_send_log starts Send log: its status on the screen, the radio on, the
// dash on the air for the phone.
void ble_send_log(state_t *state, datalog_t *log);
// ble_sending is true from ble_send_log until it's over: the phone got it
// all, or a click.
bool ble_sending(void);
void ble_poll_radio(uint32_t now_ms);
// ble_handle_click ends Send log, if it's on, and reports whether it was.
bool ble_handle_click(void);
