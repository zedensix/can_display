#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nmea.h"

// The GPS module on GPS_UART: received on interrupts, parsed in the main
// loop. gps_init sets a u-blox module to 10 fixes a second, RMC only, at
// 115200 baud (u-blox commands: another make ignores them, and must
// already send RMC at 115200 baud).
void gps_init(void);

// gps_poll returns true with the next fix received, false once there's
// none left: call it until false.
bool gps_poll(gps_fix_t *fix);
