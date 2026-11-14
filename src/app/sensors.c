#include "sensors.h"

float sensors_raw_to_volts(uint16_t raw, float vref) {
    return (float)raw / 4095 * vref;
}

float divider_ratio(divider_t d) {
    return d.r2 / (d.r1 + d.r2);
}

float divider_undo(divider_t d, float measured_volts) {
    return measured_volts / divider_ratio(d);
}

bool sensors_on_ecu_adc_grid(float volts) {
    float steps = volts / ECU_ADC_STEP_VOLTS;
    float off = steps - (float)(long)(steps + 0.5f);
    return off > -0.005f && off < 0.005f;
}

float sensors_ethanol_percent(float sensor_volts) {
    float span = ETHANOL_SENSOR_MAX_VOLTS - ETHANOL_SENSOR_MIN_VOLTS;
    float pct = (sensor_volts - ETHANOL_SENSOR_MIN_VOLTS) / span * 100;
    if (pct < 0) {
        return 0;
    }
    if (pct > 100) {
        return 100;
    }
    return pct;
}
