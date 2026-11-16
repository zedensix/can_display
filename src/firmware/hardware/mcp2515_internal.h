#pragma once

// What mcp2515.c (the driver) and mcp2515_selftest.c share: the SPI
// instructions, the registers, and the transfers.

#include <stddef.h>
#include <stdint.h>

#include "data/signals.h"
#include "obd/obd.h"

enum {
    INSTR_RESET = 0xC0,
    INSTR_WRITE = 0x02,
    INSTR_READ = 0x03,
    INSTR_BIT_MODIFY = 0x05,
    INSTR_READ_STATUS = 0xA0,
    INSTR_READ_RX0 = 0x90,
    INSTR_READ_RX1 = 0x94,
    INSTR_LOAD_TX0 = 0x40,
    INSTR_RTS_TX0 = 0x81,

    REG_CANSTAT = 0x0E,
    REG_CANCTRL = 0x0F,
    REG_CANINTF = 0x2C,
    REG_CNF3 = 0x28,
    REG_CNF2 = 0x29,
    REG_CNF1 = 0x2A,
    REG_TXB0D0 = 0x36, // TX buffer 0's first data byte: a free register for the readback
    REG_RXB0CTRL = 0x60,
    REG_RXB1CTRL = 0x70,

    // CANCTRL: the mode asked for, in its top 3 bits.
    CANCTRL_NORMAL = 0x00,
    CANCTRL_LOOPBACK = 0x40,
    CANCTRL_LISTEN = 0x60,

    // CANINTF and READ STATUS: a frame in RX buffer 0 or 1; CANINTF: a receive error.
    RX0_FULL = 0x01,
    RX1_FULL = 0x02,
    CANINTF_MERRF = 0x80,
};

void mcp2515_transfer(const uint8_t *out, uint8_t *in, size_t len);
void mcp2515_write(uint8_t reg, uint8_t value);
uint8_t mcp2515_read(uint8_t reg);
// mcp2515_read_status is READ STATUS: which RX buffers hold a frame (RX0_FULL, RX1_FULL).
uint8_t mcp2515_read_status(void);
// mcp2515_read_frame reads an RX buffer (INSTR_READ_RX0 or 1), which frees
// it; false for an extended (29bit) frame.
bool mcp2515_read_frame(uint8_t instruction, can_frame_t *f);
// mcp2515_send puts a standard frame in TX buffer 0 and sends it.
void mcp2515_send(const can_frame_t *f);

// mcp2515_selftest_start resets the chip and runs the SPI and loopback
// checks (a few milliseconds, at boot), then leaves it listening, silent,
// with its bit timing set. False on a failure, its status on the panel.
bool mcp2515_selftest_start(state_t *state);
// mcp2515_selftest_listen reads the bus without a word sent and reports on
// the panel; true once it passed (frames decoded meanwhile, as usual).
bool mcp2515_selftest_listen(state_t *state, uint32_t now_ms);
