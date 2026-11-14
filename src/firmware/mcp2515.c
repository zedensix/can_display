#include "mcp2515.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "obd.h"
#include "pico/time.h"
#include "store.h"

// 1MHz is conservative on purpose: plenty for dashboard-rate data, and it
// survives cheap level shifters and long jumper wires.
#define CAN_BAUD 1000000
#define AFR_INTERVAL_MS 250

enum {
    INSTR_RESET = 0xC0,
    INSTR_WRITE = 0x02,
    INSTR_READ = 0x03,
    INSTR_READ_STATUS = 0xA0,
    INSTR_READ_RX0 = 0x90,
    INSTR_READ_RX1 = 0x94,
    INSTR_LOAD_TX0 = 0x40,
    INSTR_RTS_TX0 = 0x81,

    REG_CANSTAT = 0x0E,
    REG_CANCTRL = 0x0F,
    REG_CNF3 = 0x28,
    REG_CNF2 = 0x29,
    REG_CNF1 = 0x2A,
    REG_RXB0CTRL = 0x60,
    REG_RXB1CTRL = 0x70,
};

static state_t *state;
static bool ready;
static uint32_t afr_ms;

static void transfer(const uint8_t *out, uint8_t *in, size_t len) {
    gpio_put(CAN_CS, 0);
    if (in) {
        spi_write_read_blocking(spi1, out, in, len);
    } else {
        spi_write_blocking(spi1, out, len);
    }
    gpio_put(CAN_CS, 1);
}

static void write_register(uint8_t reg, uint8_t value) {
    transfer((const uint8_t[]){INSTR_WRITE, reg, value}, NULL, 3);
}

static uint8_t read_register(uint8_t reg) {
    uint8_t in[3];
    transfer((const uint8_t[]){INSTR_READ, reg, 0}, in, 3);
    return in[2];
}

void mcp2515_source_init(state_t *s) {
    state = s;
    spi_init(spi1, CAN_BAUD);
    gpio_set_function(CAN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CAN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(CAN_MISO, GPIO_FUNC_SPI);
    gpio_init(CAN_CS);
    gpio_set_dir(CAN_CS, GPIO_OUT);
    gpio_put(CAN_CS, 1);

    transfer((const uint8_t[]){INSTR_RESET}, NULL, 1);
    sleep_ms(10);
    // 500kbps, the GT86's high-speed CAN bus, with the 16MHz crystal most
    // breakout boards ship with. An 8MHz crystal needs other CNF values.
    write_register(REG_CNF1, 0x00);
    write_register(REG_CNF2, 0xF0);
    write_register(REG_CNF3, 0x86);
    write_register(REG_RXB0CTRL, 0x64); // any frame, roll over into RXB1
    write_register(REG_RXB1CTRL, 0x60); // any frame
    write_register(REG_CANCTRL, 0x00);  // normal mode
    sleep_ms(10);
    ready = (read_register(REG_CANSTAT) >> 5) == 0;
    if (!ready) {
        log_line("fatal: CAN: the MCP2515 didn't enter normal mode, check its wiring");
        state_set_status(state, "CAN\nMCP2515 not answering\ncheck its wiring");
        return;
    }
    log_line("can: MCP2515 ready");
}

static void receive(uint8_t instruction, uint32_t now_ms) {
    uint8_t out[14] = {instruction}, in[14];
    transfer(out, in, sizeof in);
    // in[1..4]: SIDH SIDL EID8 EID0, in[5]: DLC, in[6..13]: data. Reading
    // through READ RX clears the buffer's interrupt flag.
    if (in[2] & 0x08) {
        return; // extended (29bit) ID
    }
    can_frame_t f = {.id = (uint32_t)(in[1] << 3 | in[2] >> 5), .dlc = (uint8_t)(in[5] & 0x0F)};
    if (f.dlc > 8) {
        f.dlc = 8;
    }
    for (int i = 0; i < f.dlc; i++) {
        f.data[i] = in[6 + i];
    }
    if (!gt86_decode(&f, state, now_ms)) {
        obd_decode(&f, state, now_ms);
    }
}

static void send(const can_frame_t *f) {
    uint8_t out[14] = {INSTR_LOAD_TX0, (uint8_t)(f->id >> 3), (uint8_t)((f->id & 7) << 5), 0, 0, f->dlc};
    for (int i = 0; i < f->dlc; i++) {
        out[6 + i] = f->data[i];
    }
    transfer(out, NULL, 6 + f->dlc);
    transfer((const uint8_t[]){INSTR_RTS_TX0}, NULL, 1);
}

void mcp2515_source_poll(uint32_t now_ms) {
    if (!ready) {
        return;
    }
    uint8_t in[2];
    transfer((const uint8_t[]){INSTR_READ_STATUS, 0}, in, 2);
    if (in[1] & 0x01) {
        receive(INSTR_READ_RX0, now_ms);
    }
    if (in[1] & 0x02) {
        receive(INSTR_READ_RX1, now_ms);
    }
    // The AFR needs a request: it isn't broadcast.
    if (now_ms - afr_ms >= AFR_INTERVAL_MS) {
        afr_ms = now_ms;
        can_frame_t request = obd_request(PID_LAMBDA);
        send(&request);
    }
}
