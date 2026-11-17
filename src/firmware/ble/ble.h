#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"
#include "store/datalog.h"
#include "store/settings.h"
#include "ui/screen.h"

// The ELM327 Bluetooth LE OBD dongle as the data source: scan, connect,
// find its serial-port characteristics, then run an elm327 session over
// them. Everything runs from ble_poll_radio, in the main loop, along with the
// BTstack run loop (the cyw43 "poll" architecture: no interrupts racing
// the main loop).
//
// Without auto, it first shows "press to connect" and waits for a click
// (ble_handle_click). A failure shows what went wrong and waits for a
// click to retry, or retries by itself after 5s with auto. A retry is a
// reboot that connects right away: a clean start is the known-good way
// back, whatever state the BLE stack is in.
// ble_init starts the source. With auto_connect, it connects to the first
// dongle it finds; without, the radio scans in the background while the
// prompt waits for a click, and names the dongle it will connect to:
// last, the one last connected to, if it's around.
void ble_init(state_t *state, bool auto_connect, const dongle_t *last);

// ble_take_connected_dongle hands over, once, the dongle just connected to: the
// settings keep it as the last one.
bool ble_take_connected_dongle(dongle_t *out);
void ble_poll_radio(uint32_t now_ms);

// ble_handle_click hands a click to a waiting prompt, with the button it
// was on (screen_t.choice), and reports whether one was waiting.
bool ble_handle_click(uint32_t now_ms, choice_t choice);

// ble_request_can_listen starts a guided listen of the CAN bus, once the
// session polls (elm_request_can_listen).
void ble_request_can_listen(void);

// ble_hold_retries holds the automatic retry after a failure while hold
// (the settings page is open: the driver may be switching to CAN).
void ble_hold_retries(bool hold);

// ble_in_standby is true once the car is off: the engine ECU stayed silent
// for CAR_OFF_MS. The dongle is let go so it can sleep, the radio is off,
// and a click wakes everything up (a reboot that connects).
#define CAR_OFF_MS 90000
bool ble_in_standby(void);

// ble_request_ram_scan starts a scan of the engine ECU's RAM, once the session
// polls (elm_request_ram_scan).
void ble_request_ram_scan(void);

// ble_send_log lets the dongle go and advertises the dash to a phone,
// which gets log (see web/). Once the phone says it got it all, or on a
// click, the dash goes back to the dongle.
void ble_send_log(datalog_t *log);

// ble_go_offline lets the dongle go and turns the radio off, until a
// click (ble_handle_click) reconnects: Disconnect, on the settings page.
void ble_go_offline(void);

// ble_disconnect_before_reboot closes the dongle connection before a reboot: a dongle
// left on a connection that vanished can stop answering for a while.
void ble_disconnect_before_reboot(void);

// reboot_to_connect is in main.c: a reboot whose next boot connects
// without asking.
void reboot_to_connect(void);
