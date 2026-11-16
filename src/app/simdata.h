#pragma once

#include <stdint.h>

#include "signals.h"

// simdata_update fills state with plausible fake values at now_ms: an
// engine warming up over 90 seconds and revving in slow waves, a charging
// battery, a swinging AFR, lean (fuel cut) off the throttle, E10. For the simulator and the `sim` firmware.
void simdata_update(state_t *state, uint32_t now_ms);
