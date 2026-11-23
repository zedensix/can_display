#pragma once

#include <stdint.h>

#include "data/signals.h"

// The live readings the dash sends a phone (the Android Auto app, android/)
// many times a second over BLE, each fresh or "none", as the gauges page
// would show them: LIVE_SIZE bytes, little-endian, in one notification of
// the default ATT MTU. The app's decoder (Live.kt) must follow this layout.
//
//  0     LIVE_VERSION
//  1-2   oil, int16, tenths of a degree C (INT16_MIN: none)
//  3-4   coolant, the same
//  5     intake, int8, degrees C (INT8_MIN: none)
//  6     AFR, uint8, tenths (0: none)
//  7     battery, uint8, tenths of a volt (0: none)
//  8     ethanol, uint8, % (0xFF: none)
//  9-10  RPM, uint16 (0xFFFF: none)
//  11    gear, uint8, 1 to 6, 0 for none (state_gear)
//  12    outside, int8, degrees C (INT8_MIN: none)
//  13-15 distance, uint24, tenths of a km: the odometer, or the trip since
//        boot (LIVE_TRIP)
//  16    the drive mode (drive_mode_t) in bits 0-2, LIVE_OFF_THROTTLE,
//        LIVE_TRIP
// The app has its own settings (the shift light's RPM): none go along.
#define LIVE_VERSION 1
#define LIVE_SIZE 17
#define LIVE_OFF_THROTTLE 0x08
#define LIVE_TRIP 0x10

void live_encode(const state_t *s, uint32_t now_ms, uint8_t out[LIVE_SIZE]);
