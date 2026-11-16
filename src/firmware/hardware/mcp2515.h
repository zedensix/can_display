#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"

// The MCP2515 CAN controller on SPI1, wired to the OBD port: listens to
// the GT86's broadcast frames, and asks the engine ECU for the AFR and
// the throttle in turn. Each boot starts with a self test of its wiring
// (mcp2515_selftest.c), its findings on the panel; it transmits nothing
// before the car's bus has passed. Untested on hardware: see
// docs/wiring.md for the SO divider and the crystal.
void mcp2515_source_init(state_t *state);
void mcp2515_poll_bus(uint32_t now_ms);
// mcp2515_wants_retry is true while the panel offers Retry: the self test
// failed, or the bus is silent or garbled so far.
bool mcp2515_wants_retry(void);
