#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "signals.h"

// The MCP2515 CAN controller on SPI1, wired to the OBD port: listens to
// the GT86's broadcast frames, and asks the engine ECU for the AFR
// (speculative PID 0x24) every 250ms. Untested on hardware: see
// docs/wiring.md for the level shifting and the crystal.
void mcp2515_source_init(state_t *state);
void mcp2515_source_poll(uint32_t now_ms);
