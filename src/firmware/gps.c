#include "gps.h"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#define GPS_BAUD 115200
#define GPS_DEFAULT_BAUD 9600

// At 115200 baud the UART's 32-byte FIFO fills in 3ms, a frame takes
// tens: the interrupt moves bytes into this ring.
static volatile uint8_t ring[1024];
static volatile uint16_t head, tail;
static char line[100];
static size_t line_len;

static void on_rx(void) {
    while (uart_is_readable(GPS_UART)) {
        uint8_t c = (uint8_t)uart_getc(GPS_UART);
        uint16_t next = (uint16_t)((head + 1) % sizeof ring);
        if (next != tail) {
            ring[head] = c;
            head = next;
        }
    }
}

// send_nmea sends body ("PUBX,...") as "$body*CS\r\n".
static void send_nmea(const char *body) {
    uint8_t sum = 0;
    for (const char *p = body; *p; p++) {
        sum ^= (uint8_t)*p;
    }
    char out[80];
    snprintf(out, sizeof out, "$%s*%02X\r\n", body, sum);
    uart_puts(GPS_UART, out);
}

// configure sends a u-blox module its setup at the current baud rate:
// every NMEA sentence off but RMC, then the port to GPS_BAUD.
static void configure(void) {
    static const char *const off[] = {"GGA", "GLL", "GSA", "GSV", "VTG"};
    for (size_t i = 0; i < sizeof off / sizeof off[0]; i++) {
        char body[40];
        snprintf(body, sizeof body, "PUBX,40,%s,0,0,0,0,0,0", off[i]);
        send_nmea(body);
    }
    send_nmea("PUBX,41,1,0007,0003,115200,0");
    uart_tx_wait_blocking(GPS_UART);
    sleep_ms(100);
}

void gps_init(void) {
    uart_init(GPS_UART, GPS_DEFAULT_BAUD);
    gpio_set_function(GPS_TX, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX, GPIO_FUNC_UART);
    // Without a module, RX would float and feed the interrupt noise.
    gpio_pull_up(GPS_RX);
    // A module fresh from power-up talks at 9600; one the Pico set up
    // before a reboot of its own already at 115200. Both get the setup.
    configure();
    uart_set_baudrate(GPS_UART, GPS_BAUD);
    configure();
    // UBX CFG-RATE: a fix every 100ms (navigation rate 1, GPS time).
    static const uint8_t rate_10hz[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12};
    uart_write_blocking(GPS_UART, rate_10hz, sizeof rate_10hz);

    int irq = GPS_UART == uart0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, on_rx);
    irq_set_enabled(irq, true);
    uart_set_irq_enables(GPS_UART, true, false);
}

bool gps_poll(gps_fix_t *fix) {
    while (tail != head) {
        char c = (char)ring[tail];
        tail = (uint16_t)((tail + 1) % sizeof ring);
        if (c == '\r' || c == '\n') {
            line[line_len] = 0;
            bool parsed = line_len > 0 && nmea_parse_rmc(line, fix);
            line_len = 0;
            if (parsed) {
                return true;
            }
        } else if (line_len < sizeof line - 1) {
            line[line_len++] = c;
        }
    }
    return false;
}
