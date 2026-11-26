#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "store/datalog.h"
#include "store/settings.h"
#include "store/update.h"

// The firmware's room, from the start of the flash: the data log is past
// it, and an update's image must fit in it.
#define STORE_FIRMWARE_ROOM (1024 * 1024)

// Flash storage, from the end of the 2MB: BTstack's own two sectors, then
// a 64KB ring log, then a settings sector, then the data log down to the
// first megabyte. Placed at the end, so reflashing keeps them; store_init
// turns storage off if the firmware ever reaches into them.

void store_init(void);

// log_line prints a line on the USB serial console and appends it to the
// flash log, prefixed with the time since boot. Only for events (boot,
// the CAN self test, Send log, failures), never per frame or per OBD reply: every line is a
// flash write that stalls the CPU with interrupts off, and wears the
// flash.
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// store_dump_log_on_request prints the whole flash log on the serial console when
// it has received 'l' (see the logs mise task). Called by the main loop.
void store_dump_log_on_request(void);

// store_get_datalog is the data log, or NULL if flash storage is off.
datalog_t *store_get_datalog(void);

settings_t store_load_settings(void);
void store_save_settings(settings_t cfg);

// store_firmware_crc is the running firmware's CRC-32 and its size, the
// bytes from the start of the flash to its end (the build's .bin): worked
// out at the first call.
uint32_t store_firmware_crc(uint32_t *size);

// store_install copies u's staged image over the firmware, from RAM with
// everything else stopped, and restarts the board: about 10s with the
// screen as it was, a power cut meanwhile leaving no firmware to boot
// (BOOTSEL then).
void store_install(const update_t *u) __attribute__((noreturn));
