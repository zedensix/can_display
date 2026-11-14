#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "datalog.h"
#include "settings.h"

// Flash storage, from the end of the 2MB: BTstack's own two sectors, then
// a 64KB ring log, then a settings sector, then the data log down to the
// first megabyte. Placed at the end, so reflashing keeps them; store_init
// turns storage off if the firmware ever reaches into them.

void store_init(void);

// log_line prints a line on the USB serial console and appends it to the
// flash log, prefixed with the time since boot. Only for events (boot,
// BLE steps, failures), never per frame or per OBD reply: every line is a
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
