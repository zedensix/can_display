#pragma once

#include <stdint.h>

// Pure conversion math for the locally wired analog sensors. No
// hardware access.

// sensors_raw_to_volts converts a 12-bit RP2040 ADC reading.
float sensors_raw_to_volts(uint16_t raw, float vref);

// divider_t is a resistor divider: R1 from the sensor to the ADC pin,
// R2 from the ADC pin to ground.
typedef struct {
    float r1, r2;
} divider_t;

float divider_ratio(divider_t d);

// divider_undo turns the voltage at the ADC pin back into the sensor's.
float divider_undo(divider_t d, float measured_volts);

// ETHANOL_DIVIDER brings the flex-fuel sensor's 0.5-4.5V under the
// RP2040's 3.3V: 4.5V comes out at about 3.09V.
#define ETHANOL_DIVIDER ((divider_t){.r1 = 10000, .r2 = 22000})
#define ETHANOL_SENSOR_MIN_VOLTS 0.5f
#define ETHANOL_SENSOR_MAX_VOLTS 4.5f

// sensors_ethanol_percent maps the sensor's output linearly onto 0-100%,
// clamped: 25%/V - 12.5%, the same as the "Ethanol Sensor Scaling" of
// this car's flex-fuel tune (checked 2026-11-14), so the dash shows what
// the ECU uses. The Pico taps the same 0.5-4.5V signal the ECU reads.
float sensors_ethanol_percent(float sensor_volts);
