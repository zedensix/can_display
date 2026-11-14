#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "signals.h"

// The ELM327 Bluetooth LE OBD dongle as the data source: scan, connect,
// find its serial-port characteristics, then run an elm327 session over
// them. Everything runs from ble_poll, in the main loop, along with the
// BTstack run loop (the cyw43 "poll" architecture: no interrupts racing
// the main loop).
//
// Without auto, it first shows "press to connect" and waits for a click
// (ble_prompt_click). A failure shows what went wrong and waits for a
// click to retry, or retries by itself after 5s with auto. A retry is a
// reboot that connects right away: a clean start is the known-good way
// back, whatever state the BLE stack is in.
void ble_init(state_t *state, bool auto_connect);
void ble_poll(uint32_t now_ms);

// ble_prompt_click hands a click to a waiting prompt, and reports whether
// one was waiting.
bool ble_prompt_click(uint32_t now_ms);

// ble_standby is true once the car is off: the engine ECU stayed silent
// for CAR_OFF_MS. The dongle is let go so it can sleep, the radio is off,
// and a click wakes everything up (a reboot that connects).
#define CAR_OFF_MS 90000
bool ble_standby(void);

// ble_scan_ram starts a scan of the engine ECU's RAM, once the session
// polls (elm_scan_ram).
void ble_scan_ram(void);

// ble_listen_can starts listening to the CAN bus for a button's ID, once
// the session polls (elm_listen_can).
void ble_listen_can(void);

// ble_disconnect closes the dongle connection before a reboot: a dongle
// left on a connection that vanished can stop answering for a while.
void ble_disconnect(void);

// reboot_to_connect is in main.c: a reboot whose next boot connects
// without asking.
void reboot_to_connect(void);
