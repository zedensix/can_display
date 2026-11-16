#include "mcp2515.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "mcp2515_internal.h"
#include "obd/mcp_check.h"
#include "pico/time.h"
#include "store.h"

// 1MHz is conservative on purpose: plenty for dashboard-rate data, and it
// survives the SO line's resistor divider and long jumper wires.
#define CAN_BAUD 1000000
// What isn't broadcast needs requests, one at a time (TX0 holds a single
// frame), every REQUEST_INTERVAL_MS: a cycle asks for each of
// fast_pids, then the next of slow_pids. The answers come back by PID,
// whenever the ECU sends them.
#define REQUEST_INTERVAL_MS 40
static const uint8_t fast_pids[] = {PID_LAMBDA, PID_THROTTLE, PID_PEDAL, PID_LOAD};
static const uint8_t slow_pids[] = {PID_IAT,        PID_TIMING, PID_STFT,  PID_LTFT,
                                    PID_AFR_TARGET, PID_RAIL,   PID_SPEED, PID_AMBIENT};
#define FAST_COUNT (int)(sizeof fast_pids / sizeof fast_pids[0])
#define SLOW_COUNT (int)(sizeof slow_pids / sizeof slow_pids[0])

// Where the source stands: the self test failed (nothing more until a
// reboot), it listens to the bus, or the chip is in normal mode.
static enum { CAN_FAILED, CAN_LISTENING, CAN_RUNNING } phase;
static state_t *state;
static uint32_t request_ms;
static int request_step, slow_step;

void mcp2515_transfer(const uint8_t *out, uint8_t *in, size_t len) {
    gpio_put(CAN_CS, 0);
    if (in) {
        spi_write_read_blocking(spi1, out, in, len);
    } else {
        spi_write_blocking(spi1, out, len);
    }
    gpio_put(CAN_CS, 1);
}

void mcp2515_write(uint8_t reg, uint8_t value) {
    mcp2515_transfer((const uint8_t[]){INSTR_WRITE, reg, value}, NULL, 3);
}

uint8_t mcp2515_read(uint8_t reg) {
    uint8_t in[3];
    mcp2515_transfer((const uint8_t[]){INSTR_READ, reg, 0}, in, 3);
    return in[2];
}

uint8_t mcp2515_read_status(void) {
    uint8_t in[2];
    mcp2515_transfer((const uint8_t[]){INSTR_READ_STATUS, 0}, in, 2);
    return in[1];
}

bool mcp2515_read_frame(uint8_t instruction, can_frame_t *f) {
    uint8_t out[14] = {instruction}, in[14];
    mcp2515_transfer(out, in, sizeof in);
    // in[1..4]: SIDH SIDL EID8 EID0, in[5]: DLC, in[6..13]: data. Reading
    // through READ RX clears the buffer's interrupt flag.
    if (in[2] & 0x08) {
        return false; // extended (29bit) ID
    }
    *f = (can_frame_t){.id = (uint32_t)(in[1] << 3 | in[2] >> 5), .dlc = (uint8_t)(in[5] & 0x0F)};
    if (f->dlc > 8) {
        f->dlc = 8;
    }
    for (int i = 0; i < f->dlc; i++) {
        f->data[i] = in[6 + i];
    }
    return true;
}

void mcp2515_send(const can_frame_t *f) {
    uint8_t out[14] = {INSTR_LOAD_TX0, (uint8_t)(f->id >> 3), (uint8_t)((f->id & 7) << 5), 0, 0, f->dlc};
    for (int i = 0; i < f->dlc; i++) {
        out[6 + i] = f->data[i];
    }
    mcp2515_transfer(out, NULL, 6 + f->dlc);
    mcp2515_transfer((const uint8_t[]){INSTR_RTS_TX0}, NULL, 1);
}

void mcp2515_source_init(state_t *s) {
    state = s;
    spi_init(spi1, CAN_BAUD);
    gpio_set_function(CAN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CAN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(CAN_MISO, GPIO_FUNC_SPI);
    // An SO that answers nothing reads 0x00, not whatever a floating pin picks up.
    gpio_pull_down(CAN_MISO);
    gpio_init(CAN_CS);
    gpio_set_dir(CAN_CS, GPIO_OUT);
    gpio_put(CAN_CS, 1);
    phase = mcp2515_selftest_start(state) ? CAN_LISTENING : CAN_FAILED;
}

bool mcp2515_wants_retry(void) {
    return state && state->wait == WAIT_RETRY;
}

// start_running leaves listen-only for normal mode, where the chip
// acknowledges frames and sends the OBD requests.
static void start_running(void) {
    mcp2515_write(REG_CANCTRL, CANCTRL_NORMAL);
    sleep_ms(5);
    const char *fault = mcp_check_mode(mcp2515_read(REG_CANSTAT), MCP_MODE_NORMAL);
    if (fault) {
        log_line("can: self test failed: %s", fault);
        state_set_status(state, fault);
        state->wait = WAIT_RETRY;
        phase = CAN_FAILED;
        return;
    }
    log_line("can: self test passed, MCP2515 running");
    phase = CAN_RUNNING;
}

void mcp2515_poll_bus(uint32_t now_ms) {
    if (phase == CAN_FAILED) {
        return;
    }
    if (phase == CAN_LISTENING) {
        if (mcp2515_selftest_listen(state, now_ms)) {
            start_running();
        }
        return;
    }
    uint8_t status = mcp2515_read_status();
    can_frame_t f;
    for (int i = 0; i < 2; i++) {
        if (status & (i ? RX1_FULL : RX0_FULL) && mcp2515_read_frame(i ? INSTR_READ_RX1 : INSTR_READ_RX0, &f) &&
            !gt86_decode_broadcast(&f, state, now_ms)) {
            obd_decode_frame(&f, state, now_ms);
        }
    }
    if (now_ms - request_ms >= REQUEST_INTERVAL_MS) {
        request_ms = now_ms;
        uint8_t pid = request_step < FAST_COUNT ? fast_pids[request_step] : slow_pids[slow_step++ % SLOW_COUNT];
        request_step = (request_step + 1) % (FAST_COUNT + 1);
        can_frame_t request = obd_build_request(pid);
        mcp2515_send(&request);
    }
}
