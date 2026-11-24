#pragma once

#include <stdint.h>

#include "signals.h"

// simdata_update fills state with fake values at now_ms: a tour of the
// dash's scenarios, a scene every 4 to 10 seconds (a cold start, cruising,
// pulls to the limiter, a lift-off, the track modes, the alarms, the oil
// overheating, a low battery), about 1min40 then over again. For the
// simulator and the `sim` firmware.
void simdata_update(state_t *state, uint32_t now_ms);
