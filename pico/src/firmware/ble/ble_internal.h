#pragma once

// What ble.c and ble_update.c (a new firmware from the phone) share.

#include <stdbool.h>
#include <stdint.h>

#include "bluetooth.h"
#include "data/signals.h"

// ble_update_start: handle is the update characteristic's value (dash.h,
// which only ble.c includes: it defines the GATT database).
void ble_update_start(state_t *state, uint16_t handle);
// ble_update_command takes a write to the control characteristic if it's
// the update's ('U', 'W', 'X'), and reports whether it was; busy: Send log
// or a pull under way, an update refused.
bool ble_update_command(hci_con_handle_t con, const uint8_t *command, uint16_t length, uint32_t now, bool busy);
// ble_update_read answers a read of the update characteristic (BTstack's
// read callback), 0 for any other handle.
uint16_t ble_update_read(uint16_t handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
void ble_update_can_send(void);
void ble_update_disconnected(void);
void ble_update_poll(uint32_t now);
// ble_update_click takes a click while an update asks or runs: allow it,
// or cancel it.
bool ble_update_click(uint32_t now);
// ble_update_pulling shows an update's pull of the data log (sent of total
// bytes), the update's screen from its start to the end of the update.
void ble_update_pulling(uint32_t sent, uint32_t total, uint32_t now);
bool ble_updating(void);
