// The MCP2515's self test, on every boot with the CAN source: the SPI
// lines (an answer after a reset, a register that reads back what was
// written), the chip and its crystal (a frame sent to itself in loopback
// mode), then the car's bus, listened to in listen-only mode, which never
// transmits nor acknowledges: a wrong bit rate or swapped wires can't
// disturb the car. src/app/obd/mcp_check.c says what each reading means.

#include "board.h"
#include "mcp2515_internal.h"
#include "obd/mcp_check.h"
#include "pico/time.h"
#include "store.h"

#define LOOPBACK_ID 0x123
#define LOOPBACK_WAIT_MS 20

static void fail(state_t *state, const char *status) {
    log_line("can: self test failed: %s", status);
    state_set_status(state, status);
    state->wait = WAIT_RETRY;
}

// set_mode asks for a mode and checks CANSTAT reports it.
static const char *set_mode(uint8_t canctrl, int wanted) {
    mcp2515_write(REG_CANCTRL, canctrl);
    sleep_ms(5);
    return mcp_check_mode(mcp2515_read(REG_CANSTAT), wanted);
}

// loopback sends a frame to the chip itself and checks it comes back.
static bool loopback(void) {
    can_frame_t sent = {LOOPBACK_ID, 2, {0xA5, 0x5A}}, got = {0};
    mcp2515_send(&sent);
    for (int ms = 0; ms < LOOPBACK_WAIT_MS; ms++) {
        uint8_t status = mcp2515_read_status();
        if (status & (RX0_FULL | RX1_FULL)) {
            bool read = mcp2515_read_frame(status & RX0_FULL ? INSTR_READ_RX0 : INSTR_READ_RX1, &got);
            return read && got.id == sent.id && got.dlc == sent.dlc && got.data[0] == 0xA5 && got.data[1] == 0x5A;
        }
        sleep_ms(1);
    }
    return false;
}

bool mcp2515_selftest_start(state_t *state) {
    mcp2515_transfer((const uint8_t[]){INSTR_RESET}, NULL, 1);
    sleep_ms(10);
    uint8_t canstat = mcp2515_read(REG_CANSTAT);
    mcp2515_write(REG_TXB0D0, 0x55);
    bool readback = mcp2515_read(REG_TXB0D0) == 0x55;
    mcp2515_write(REG_TXB0D0, 0xAA);
    readback = readback && mcp2515_read(REG_TXB0D0) == 0xAA;
    log_line("can: self test: CANSTAT 0x%02X after reset, readback %s", canstat, readback ? "ok" : "wrong");
    const char *fault = mcp_check_spi(canstat, readback);
    if (fault) {
        fail(state, fault);
        return false;
    }

    // 500kbps, the GT86's high-speed CAN bus, with the 8MHz crystal of the
    // blue MCP2515 + TJA1050 board. One time quantum is 2 / 8MHz = 250ns,
    // a bit is 8 of them: sync 1, propagation 2, phase 1 3, phase 2 2,
    // sampled at 75%. SJW 1, no prescaler. A 16MHz crystal takes 0x00,
    // 0xF0, 0x86 instead.
    mcp2515_write(REG_CNF1, 0x00);     // SJW 1, BRP 0
    mcp2515_write(REG_CNF2, 0x91);     // phase 2 set in CNF3, phase 1 3, propagation 2
    mcp2515_write(REG_CNF3, 0x01);     // phase 2 2
    mcp2515_write(REG_RXB0CTRL, 0x64); // any frame, roll over into RXB1
    mcp2515_write(REG_RXB1CTRL, 0x60); // any frame

    fault = set_mode(CANCTRL_LOOPBACK, MCP_MODE_LOOPBACK);
    if (!fault && !loopback()) {
        fault = "CAN SELF TEST\nloopback frame lost\nthe MCP2515 itself, or its crystal";
    }
    if (!fault) {
        fault = set_mode(CANCTRL_LISTEN, MCP_MODE_LISTEN);
    }
    if (fault) {
        fail(state, fault);
        return false;
    }
    log_line("can: self test: SPI and loopback ok, listening");
    state_set_status(state, mcp_bus_status(BUS_LISTENING));
    state->wait = WAIT_NONE;
    return true;
}

bool mcp2515_selftest_listen(state_t *state, uint32_t now_ms) {
    static uint32_t since;
    static int frames, errors;
    static bus_verdict_t shown = BUS_LISTENING;
    if (!since) {
        since = now_ms ? now_ms : 1;
    }

    uint8_t status = mcp2515_read_status();
    for (int i = 0; i < 2; i++) {
        uint8_t full = i ? RX1_FULL : RX0_FULL;
        can_frame_t f;
        if (status & full && mcp2515_read_frame(i ? INSTR_READ_RX1 : INSTR_READ_RX0, &f)) {
            frames++;
            if (!gt86_decode_broadcast(&f, state, now_ms)) {
                obd_decode_frame(&f, state, now_ms);
            }
        }
    }
    if (mcp2515_read(REG_CANINTF) & CANINTF_MERRF) {
        errors++;
        mcp2515_transfer((const uint8_t[]){INSTR_BIT_MODIFY, REG_CANINTF, CANINTF_MERRF, 0}, NULL, 4);
    }

    bus_verdict_t verdict = mcp_check_bus(frames, errors, now_ms - since);
    if (verdict == shown) {
        return false;
    }
    shown = verdict;
    log_line("can: self test: bus %s, %d frames, %d receive errors in %lu ms",
             verdict == BUS_OK        ? "ok"
             : verdict == BUS_SILENT  ? "silent"
             : verdict == BUS_GARBLED ? "garbled"
                                      : "listening",
             frames, errors, (unsigned long)(now_ms - since));
    state_set_status(state, mcp_bus_status(verdict));
    // A silent bus is the car asleep far more often than a wire: no Retry,
    // the listen goes on and passes once the ignition wakes the bus.
    state->wait = verdict == BUS_GARBLED ? WAIT_RETRY : WAIT_NONE;
    return verdict == BUS_OK;
}
