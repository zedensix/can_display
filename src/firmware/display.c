#include "display.h"

#include "board.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "pico/time.h"

#define DISPLAY_BAUD 62500000

enum {
    CMD_SWRESET = 0x01,
    CMD_SLPIN = 0x10,
    CMD_SLPOUT = 0x11,
    CMD_NORON = 0x13,
    CMD_INVON = 0x21,
    CMD_DISPON = 0x29,
    CMD_CASET = 0x2A,
    CMD_RASET = 0x2B,
    CMD_RAMWR = 0x2C,
    CMD_MADCTL = 0x36,
    CMD_COLMOD = 0x3A,
    CMD_PORCTRL = 0xB2,
    CMD_FRCTRL2 = 0xC6,
};

#define MADCTL_MX 0x40
#define MADCTL_MY 0x80

static int dma_channel;
static bool pushing;

static void command(uint8_t cmd, const uint8_t *data, size_t len) {
    gpio_put(DISPLAY_CS, 0);
    gpio_put(DISPLAY_DC, 0);
    spi_write_blocking(spi0, &cmd, 1);
    if (len) {
        gpio_put(DISPLAY_DC, 1);
        spi_write_blocking(spi0, data, len);
    }
    gpio_put(DISPLAY_CS, 1);
}

void display_init(void) {
    spi_init(spi0, DISPLAY_BAUD);
    gpio_set_function(DISPLAY_SCK, GPIO_FUNC_SPI);
    gpio_set_function(DISPLAY_MOSI, GPIO_FUNC_SPI);
    const uint pins[] = {DISPLAY_CS, DISPLAY_DC, DISPLAY_RESET};
    for (size_t i = 0; i < sizeof pins / sizeof pins[0]; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 1);
    }

    gpio_put(DISPLAY_RESET, 0);
    sleep_ms(50);
    gpio_put(DISPLAY_RESET, 1);
    sleep_ms(50);
    command(CMD_SWRESET, NULL, 0);
    sleep_ms(150);
    command(CMD_SLPOUT, NULL, 0);
    command(CMD_COLMOD, (const uint8_t[]){0x55}, 1); // RGB565
    sleep_ms(10);
    display_set_flip(false);
    command(CMD_FRCTRL2, (const uint8_t[]){0x0F}, 1); // 60Hz
    command(CMD_PORCTRL, (const uint8_t[]){8, 8, 0x00, 0x22, 0x22}, 5);
    command(CMD_INVON, NULL, 0); // these IPS panels show inverted colors without it
    command(CMD_NORON, NULL, 0);
    command(CMD_DISPON, NULL, 0);
    sleep_ms(10);

    // The backlight is dimmed by PWM at 1kHz: 125MHz / 125 / 1000.
    gpio_set_function(DISPLAY_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(DISPLAY_BL);
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_wrap(slice, 999);
    pwm_set_gpio_level(DISPLAY_BL, 1000);
    pwm_set_enabled(slice, true);

    dma_channel = dma_claim_unused_channel(true);
}

void display_set_flip(bool flip) {
    command(CMD_MADCTL, (const uint8_t[]){flip ? MADCTL_MX | MADCTL_MY : 0}, 1);
}

void display_sleep(void) {
    display_finish();
    pwm_set_gpio_level(DISPLAY_BL, 0);
    command(CMD_SLPIN, NULL, 0);
}

void display_set_brightness(int percent) {
    pwm_set_gpio_level(DISPLAY_BL, (uint16_t)(percent * 10));
}

void display_push(const canvas_t *c) {
    int last_x = c->width - 1, top = c->band_top, bottom = c->band_top + c->band_height - 1;
    command(CMD_CASET, (const uint8_t[]){0, 0, (uint8_t)(last_x >> 8), (uint8_t)last_x}, 4);
    command(CMD_RASET, (const uint8_t[]){(uint8_t)(top >> 8), (uint8_t)top, (uint8_t)(bottom >> 8), (uint8_t)bottom},
            4);
    command(CMD_RAMWR, NULL, 0);

    // 16-bit SPI frames go out MSB first: the panel gets each RGB565
    // pixel big-endian, straight from the canvas.
    gpio_put(DISPLAY_CS, 0);
    gpio_put(DISPLAY_DC, 1);
    spi_set_format(spi0, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    dma_channel_config config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_dreq(&config, spi_get_dreq(spi0, true));
    dma_channel_configure(dma_channel, &config, &spi_get_hw(spi0)->dr, c->pixels,
                          (uint)(c->width * c->band_height), true);
    pushing = true;
}

bool display_busy(void) {
    return pushing && (dma_channel_is_busy(dma_channel) || spi_is_busy(spi0));
}

void display_finish(void) {
    if (!pushing) {
        return;
    }
    dma_channel_wait_for_finish_blocking(dma_channel);
    while (spi_is_busy(spi0)) {
    }
    gpio_put(DISPLAY_CS, 1);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    pushing = false;
}
