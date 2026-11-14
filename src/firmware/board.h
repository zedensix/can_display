#pragma once

// Wiring, see docs/wiring.md: these and its tables must stay in sync.
// The RP2040 has two SPI peripherals: the display gets SPI0, the MCP2515
// SPI1. GPIO23/24/25/29 are taken by the Pico W's wireless chip, and
// GPIO15 must stay free: the SDK's workaround for the RP2040 USB
// enumeration erratum (RP2040-E5) borrows it at boot.

#define DISPLAY_SCK 18
#define DISPLAY_MOSI 19
#define DISPLAY_CS 17
#define DISPLAY_DC 20
#define DISPLAY_RESET 21
#define DISPLAY_BL 22

#define CAN_SCK 10
#define CAN_MOSI 11
#define CAN_MISO 12
#define CAN_CS 13

#define KNOB_CLK 2
#define KNOB_DT 3
#define KNOB_BUTTON 4

// ADC0, fed through the divider in docs/wiring.md.
#define ETHANOL_PIN 26
#define ETHANOL_ADC_INPUT 0

// ADC_VREF is the RP2040 ADC reference (the 3.3V rail on a stock Pico W).
// Tune it against a multimeter: it typically reads a few tens of mV off.
#define ADC_VREF 3.3f

#define PANEL_WIDTH 240
#define PANEL_HEIGHT 320

// Where car data comes from. SOURCE_ELM327_BLE talks to an ELM327 OBD
// dongle over Bluetooth LE: nothing to wire into the car, but only
// standard OBD PIDs at a few Hz (no oil temperature). SOURCE_MCP2515
// listens to the CAN bus through an MCP2515 wired to the OBD port: faster,
// and oil temp from the broadcast frames. The `sim` build replaces both
// with fake data.
#define SOURCE_ELM327_BLE 1
#define SOURCE_MCP2515 2
#define DATA_SOURCE SOURCE_ELM327_BLE
