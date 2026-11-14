#include "knob.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// 4 quadrature steps per detent. If one click moves two steps, the
// encoder is a half-step one: use 2.
#define STEPS_PER_DETENT 4

static volatile int32_t steps;
static uint8_t last_ab;
static int32_t last_detents;
static uint32_t pressed_at;
static bool pressed, long_done;

// Quadrature transitions, indexed by previous AB << 2 | new AB: +1 or -1
// for a valid step, 0 for none or a bounce that skipped a state.
static const int8_t transitions[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static void knob_irq(void) {
    uint32_t clk = gpio_get_irq_event_mask(KNOB_CLK), dt = gpio_get_irq_event_mask(KNOB_DT);
    if (clk) {
        gpio_acknowledge_irq(KNOB_CLK, clk);
    }
    if (dt) {
        gpio_acknowledge_irq(KNOB_DT, dt);
    }
    uint8_t ab = (uint8_t)(gpio_get(KNOB_CLK) << 1 | gpio_get(KNOB_DT));
    steps += transitions[last_ab << 2 | ab];
    last_ab = ab;
}

void knob_init(void) {
    const uint pins[] = {KNOB_CLK, KNOB_DT, KNOB_BUTTON};
    for (size_t i = 0; i < sizeof pins / sizeof pins[0]; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        // The KY-040 has pull-ups on CLK and DT but usually not on SW.
        gpio_pull_up(pins[i]);
    }
    last_ab = (uint8_t)(gpio_get(KNOB_CLK) << 1 | gpio_get(KNOB_DT));
    gpio_add_raw_irq_handler_masked((1u << KNOB_CLK) | (1u << KNOB_DT), knob_irq);
    gpio_set_irq_enabled(KNOB_CLK, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(KNOB_DT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

int knob_poll(uint32_t now_ms, bool *press, bool *long_press) {
    int32_t s = steps;
    // Floor division, so detents don't stick around zero.
    int32_t detents = (s >= 0 ? s : s - (STEPS_PER_DETENT - 1)) / STEPS_PER_DETENT;
    int turned = (int)(detents - last_detents);
    last_detents = detents;

    *press = *long_press = false;
    bool down = !gpio_get(KNOB_BUTTON);
    if (down && !pressed) {
        pressed = true;
        pressed_at = now_ms;
    } else if (down && !long_done && now_ms - pressed_at >= KNOB_LONG_PRESS_MS) {
        long_done = *long_press = true;
    } else if (!down && pressed) {
        *press = !long_done;
        pressed = long_done = false;
    }
    return turned;
}
