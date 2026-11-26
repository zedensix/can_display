#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"
#include "store/datalog.h"

// The dash as a Bluetooth LE peripheral, "GT86 DASH" (dash.gatt), on the
// air from boot for one phone (Android lets its apps share the link): the
// Android Auto app subscribes to the live readings (live.h), a frame every
// 50ms; Send log's page writes 'S', gets a 4-byte size then the data log
// from its oldest record as notifications, and writes 'D' once it has it
// all. It runs from ble_poll_radio, in the main loop, along with the
// BTstack run loop (the cyw43 "poll" architecture: no interrupts racing
// the main loop).
//
// BTstack writes flash (its keys, in its own sectors) through the SDK's
// flash_safe_execute, whose helper (store.c) holds core1, reading the CAN
// bus from flash, in RAM meanwhile. Send log pauses the CAN source, the
// gauges and the data log with it: no record written over the ring while
// it's read out. A firmware update from the phone (ble_update.c) pauses
// them the same way.

// ble_start brings the radio and BTstack up and the dash on the air, the
// live readings from state.
void ble_start(state_t *state);
// ble_send_log starts Send log: its status on the screen until it's over.
void ble_send_log(datalog_t *log);
// ble_sending is true from ble_send_log until it's over (the phone got it
// all, or a click), and while a firmware update asks or runs
// (ble_update.c): the CAN source and the data log paused meanwhile.
bool ble_sending(void);
void ble_poll_radio(uint32_t now_ms);
// ble_handle_click allows or cancels an update, or ends Send log, if one
// is on, and reports whether it was.
bool ble_handle_click(void);
