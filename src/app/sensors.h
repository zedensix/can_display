#pragma once

#include <stdbool.h>

// The flex-fuel sensor's range and the engine ECU's ADC, as the tune
// sees them: what a RAM scan looks for. No hardware access.

// The flex-fuel sensor outputs 0.5 to 4.5V, read by the ECU as 25%/V -
// 12.5% (the tune's "Ethanol Sensor Scaling", checked 2026-11-14).
#define ETHANOL_SENSOR_MIN_VOLTS 0.5f
#define ETHANOL_SENSOR_MAX_VOLTS 4.5f

// ECU_ADC_STEP_VOLTS is one step of the engine ECU's 12-bit ADC on a 5V
// reference: every ethanol % in an Alientech log of this car's ECU is
// 25 x n x ECU_ADC_STEP_VOLTS - 12.5 (checked 2026-11-14).
#define ECU_ADC_STEP_VOLTS (5.0f / 4096)

// sensors_is_whole_ecu_adc_step tells whether volts (positive) is a whole
// number of ECU ADC steps: how a RAM scan spots the ethanol value.
bool sensors_is_whole_ecu_adc_step(float volts);
