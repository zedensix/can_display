#include "mcp2515.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "mcp2515_internal.h"
#include "obd/mcp_check.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/util/queue.h"
#include "store.h"

// 1MHz is conservative on purpose: plenty for dashboard-rate data, and it
// survives the SO line's resistor divider and long jumper wires.
#define CAN_BAUD 1000000
// What isn't broadcast needs requests, one at a time (TX0 holds a single
// frame), every REQUEST_INTERVAL_MS: a cycle asks for each of
// fast_pids, then the next of slow_pids (0: the ethanol %, a RAM read).
// The answers come back by PID, whenever the ECU sends them. The intake
// and the battery are fast: their tiles go stale after 2s. So is the
// speed: the gear (state_gear) wants it under 2s old, read beside RPM
// that comes 100 times a second; among the slow ones it came every 2.2s.
#define REQUEST_INTERVAL_MS 40
static const uint8_t fast_pids[] = {PID_LAMBDA, PID_THROTTLE, PID_PEDAL, PID_LOAD, PID_IAT, PID_VOLTAGE, PID_SPEED};
static const uint8_t slow_pids[] = {PID_TIMING, PID_STFT, PID_LTFT, PID_AFR_TARGET, PID_RAIL, PID_AMBIENT, 0};
#define FAST_COUNT (int)(sizeof fast_pids / sizeof fast_pids[0])
#define SLOW_COUNT (int)(sizeof slow_pids / sizeof slow_pids[0])

// Where the source stands: the self test failed (nothing more until a
// reboot), it listens to the bus, or the chip is in normal mode.
static enum { CAN_FAILED, CAN_LISTENING, CAN_RUNNING } phase;
static state_t *state;
// Once running, core1 (bus_reader) alone talks to the chip: it reads
// each frame into rx_queue as it comes and sends the requests; core0
// decodes what's queued. Polled from core0 between frames drawn, each
// buffer held one frame while a frame rendered: a 20Hz broadcast like
// the temperatures, its period in phase with the loop's 50ms, was lost
// every time for seconds on end.
#define RX_QUEUE_FRAMES 64
static queue_t rx_queue;
static volatile uint32_t dropped; // frames the queue had no room for
// The frames read in the first COUNT_MS of running, by ID (counted_ids),
// logged once: how often each comes through, to tell a frame seldom on
// the bus from one the polling misses.
#define COUNT_MS 60000
static const uint16_t counted_ids[] = {OBD_RESPONSE_ECU1, GT86_ID_DOORS, GT86_ID_RPM, GT86_ID_DRIVE_MODE,
                                       GT86_ID_TEMPERATURES};
#define COUNTED (int)(sizeof counted_ids / sizeof counted_ids[0])
static uint32_t counts[COUNTED], running_ms;
static bool counts_logged;

static void count_frame(uint32_t id, uint32_t now_ms) {
    if (counts_logged) {
        return;
    }
    for (int i = 0; i < COUNTED; i++) {
        counts[i] += id == counted_ids[i];
    }
    if (now_ms - running_ms >= COUNT_MS) {
        counts_logged = true;
        log_line("can: frames in the first minute: 7E8 %lu, 375 %lu, 140 %lu, 0D3 %lu, 360 %lu, %lu dropped", counts[0],
                 counts[1], counts[2], counts[3], counts[4], (unsigned long)dropped);
    }
}

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

static void set_filter(uint8_t reg, uint16_t id) {
    mcp2515_write(reg, (uint8_t)(id >> 3));
    mcp2515_write(reg + 1, (uint8_t)((id & 7) << 5));
}

// set_filters lets in only the frames read, out of the thousands a second
// on the bus: polled between frames drawn, 2 buffers would hold whatever
// came last, a rare frame (the doors, an answer) almost never. RX buffer
// 0 takes the answers and the doors, no rollover, so the fast broadcasts
// in buffer 1 can't push them out.
static void set_filters(void) {
    mcp2515_write(REG_CANCTRL, CANCTRL_CONFIG);
    sleep_ms(2);
    set_filter(REG_RXM0SIDH, 0x7FF);
    set_filter(REG_RXM1SIDH, 0x7FF);
    set_filter(REG_RXF0SIDH, OBD_RESPONSE_ECU1);
    set_filter(REG_RXF1SIDH, GT86_ID_DOORS);
    set_filter(REG_RXF2SIDH, GT86_ID_RPM);
    set_filter(REG_RXF3SIDH, GT86_ID_DRIVE_MODE);
    set_filter(REG_RXF4SIDH, GT86_ID_TEMPERATURES);
    set_filter(REG_RXF5SIDH, GT86_ID_TEMPERATURES);
    mcp2515_write(REG_RXB0CTRL, 0x00); // filtered, no rollover
    mcp2515_write(REG_RXB1CTRL, 0x00); // filtered
}

// bus_reader, on core1, reads both RX buffers as fast as SPI goes and
// sends a request every REQUEST_INTERVAL_MS. A flash write pauses it
// (multicore lockout): it runs from flash.
static void bus_reader(void) {
    multicore_lockout_victim_init();
    uint32_t request_ms = 0;
    int request_step = 0, slow_step = 0;
    for (;;) {
        uint8_t status = mcp2515_read_status();
        can_frame_t f;
        for (int i = 0; i < 2; i++) {
            if (status & (i ? RX1_FULL : RX0_FULL) && mcp2515_read_frame(i ? INSTR_READ_RX1 : INSTR_READ_RX0, &f) &&
                !queue_try_add(&rx_queue, &f)) {
                dropped++;
            }
        }
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if (now_ms - request_ms >= REQUEST_INTERVAL_MS) {
            request_ms = now_ms;
            uint8_t pid = request_step < FAST_COUNT ? fast_pids[request_step] : slow_pids[slow_step++ % SLOW_COUNT];
            request_step = (request_step + 1) % (FAST_COUNT + 1);
            can_frame_t request = pid ? obd_build_request(pid) : obd_build_ethanol_read();
            mcp2515_send(&request);
        }
    }
}

// start_running leaves listen-only for normal mode, where the chip
// acknowledges frames and sends the OBD requests, the filters set.
static void start_running(void) {
    set_filters();
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
    running_ms = to_ms_since_boot(get_absolute_time());
    queue_init(&rx_queue, sizeof(can_frame_t), RX_QUEUE_FRAMES);
    multicore_launch_core1(bus_reader);
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
    can_frame_t f;
    while (queue_try_remove(&rx_queue, &f)) {
        count_frame(f.id, now_ms);
        if (!gt86_decode_broadcast(&f, state, now_ms)) {
            obd_decode_frame(&f, state, now_ms);
        }
    }
}
