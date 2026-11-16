#include "sensors.h"

bool sensors_is_whole_ecu_adc_step(float volts) {
    float steps = volts / ECU_ADC_STEP_VOLTS;
    float off = steps - (float)(long)(steps + 0.5f);
    return off > -0.005f && off < 0.005f;
}
