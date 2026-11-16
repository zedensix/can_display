#pragma once

#include <stdbool.h>
#include <stdint.h>

// The MCP2515's self test, the part without hardware: what each step's
// readings say about the wiring, as the status screen's text. The
// firmware runs it each boot on the CAN source: SPI, then the chip's own
// loopback, then the car's bus listened to without a word sent, before
// it ever transmits.

// The modes CANSTAT reports in its top 3 bits.
enum { MCP_MODE_NORMAL = 0, MCP_MODE_LOOPBACK = 2, MCP_MODE_LISTEN = 3, MCP_MODE_CONFIG = 4 };

// mcp_check_spi says what's wrong on the SPI lines, or NULL: canstat read
// right after a reset (config mode: 0x80), and whether 0x55 then 0xAA
// written to a register read back.
const char *mcp_check_spi(uint8_t canstat, bool readback_ok);

// mcp_check_mode says what's wrong when CANSTAT doesn't report the mode
// asked for, or NULL.
const char *mcp_check_mode(uint8_t canstat, int wanted);

// The car's bus, listened to: still listening (nothing to say yet, or
// silent so far), fine, or frames that come in garbled.
typedef enum { BUS_LISTENING, BUS_SILENT, BUS_OK, BUS_GARBLED } bus_verdict_t;

#define MCP_BUS_FRAMES_OK 10      // frames received for the bus to pass
#define MCP_BUS_SILENT_MS 5000    // nothing by then: say so, and keep listening
#define MCP_BUS_ERRORS_GARBLED 20 // receive errors with no frame: garbled

// mcp_check_bus judges the bus from the frames received, the receive
// errors seen and how long it's been listened to.
bus_verdict_t mcp_check_bus(int frames, int errors, uint32_t listened_ms);

// mcp_bus_status is the status text for a verdict, out of a test that
// passed SPI and loopback (BUS_OK: none, the gauges come on).
const char *mcp_bus_status(bus_verdict_t verdict);
