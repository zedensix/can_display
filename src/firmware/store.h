#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

// Flash storage, from the end of the 2MB: BTstack's own two sectors, then
// a 64KB ring log, then a settings sector. Placed at the end, so a
// bigger firmware doesn't land on them and reflashing keeps them.

void store_init(void);

// log_line prints a line on the USB serial console and appends it to the
// flash log, prefixed with the time since boot. Only for events (boot,
// BLE steps, failures), never per frame or per OBD reply: every line is a
// flash write that stalls the CPU with interrupts off, and wears the
// flash.
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// store_serve_log prints the whole flash log on the serial console when
// it has received 'l' (see the logs mise task). Called by the main loop.
void store_serve_log(void);

settings_t store_load_settings(void);
void store_save_settings(settings_t cfg);
