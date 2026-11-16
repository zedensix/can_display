#include "mcp_check.h"

#include <stddef.h>

const char *mcp_check_spi(uint8_t canstat, bool readback_ok) {
    if (canstat == 0x00) {
        return "CAN SELF TEST\nMCP2515 not answering\ncheck its VCC (5V) and GND, CS (GP13), SCK (GP10), SI (GP11), "
               "SO (GP12)";
    }
    if (canstat == 0xFF) {
        return "CAN SELF TEST\nSO stuck high\ncheck SO (GP12) and its divider: 1k in series, 2k to GND";
    }
    if (canstat >> 5 != MCP_MODE_CONFIG) {
        return "CAN SELF TEST\nMCP2515 answers garbage\ncheck SCK (GP10) and SI (GP11), short wires";
    }
    if (!readback_ok) {
        return "CAN SELF TEST\nwrites don't stick\ncheck SI (GP11) and SCK (GP10)";
    }
    return NULL;
}

const char *mcp_check_mode(uint8_t canstat, int wanted) {
    if (canstat >> 5 == wanted) {
        return NULL;
    }
    // Leaving config mode needs the oscillator running.
    if (canstat >> 5 == MCP_MODE_CONFIG) {
        return "CAN SELF TEST\nstuck in config mode\nits crystal doesn't run";
    }
    return "CAN SELF TEST\nwrong mode\nthe MCP2515 doesn't take orders: SI (GP11)";
}

bus_verdict_t mcp_check_bus(int frames, int errors, uint32_t listened_ms) {
    if (frames >= MCP_BUS_FRAMES_OK) {
        return BUS_OK;
    }
    if (frames == 0 && errors >= MCP_BUS_ERRORS_GARBLED) {
        return BUS_GARBLED;
    }
    return listened_ms >= MCP_BUS_SILENT_MS && frames == 0 ? BUS_SILENT : BUS_LISTENING;
}

const char *mcp_bus_status(bus_verdict_t verdict) {
    switch (verdict) {
    case BUS_LISTENING:
        return "CAN SELF TEST\nSPI ok, loopback ok\nlistening to the car";
    case BUS_SILENT:
        return "CAN SELF TEST\nSPI ok, loopback ok\nno frame yet: ignition on? CAN-H on OBD 6, CAN-L on 14";
    case BUS_GARBLED:
        return "CAN SELF TEST\nframes garbled\nCAN-H and CAN-L swapped? or not an 8MHz crystal";
    case BUS_OK:
        break;
    }
    return "";
}
