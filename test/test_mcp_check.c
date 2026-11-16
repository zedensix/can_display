#include "check.h"
#include "obd/mcp_check.h"

void test_mcp_check(void) {
    CHECK(mcp_check_spi(0x80, true) == NULL, "a reset MCP2515 in config mode failed the SPI check");
    CHECK(strstr(mcp_check_spi(0x00, true), "not answering"), "0x00 should say it's not answering");
    CHECK(strstr(mcp_check_spi(0xFF, true), "SO stuck high"), "0xFF should blame SO");
    CHECK(strstr(mcp_check_spi(0x13, true), "garbage"), "a wrong mode should say garbage");
    CHECK(strstr(mcp_check_spi(0x80, false), "SI (GP11)"), "a failed readback should blame SI");

    CHECK(mcp_check_mode(0x40, MCP_MODE_LOOPBACK) == NULL, "loopback mode reported, not taken");
    CHECK(mcp_check_mode(0x60, MCP_MODE_LISTEN) == NULL, "listen-only mode reported, not taken");
    CHECK(strstr(mcp_check_mode(0x80, MCP_MODE_LOOPBACK), "crystal"), "stuck in config mode should blame the crystal");

    CHECK(mcp_check_bus(0, 0, 100) == BUS_LISTENING, "just started should be listening");
    CHECK(mcp_check_bus(0, 0, MCP_BUS_SILENT_MS) == BUS_SILENT, "nothing for 5s should be silent");
    CHECK(mcp_check_bus(3, 0, MCP_BUS_SILENT_MS) == BUS_LISTENING, "a few frames should keep listening");
    CHECK(mcp_check_bus(MCP_BUS_FRAMES_OK, 5, 200) == BUS_OK, "enough frames should pass, errors or not");
    CHECK(mcp_check_bus(0, MCP_BUS_ERRORS_GARBLED, 200) == BUS_GARBLED, "errors and no frame should be garbled");
    CHECK(mcp_bus_status(BUS_OK)[0] == '\0', "a bus that passed should clear the status");
    CHECK(strstr(mcp_bus_status(BUS_GARBLED), "swapped"), "garbled should suggest swapped wires");
}
