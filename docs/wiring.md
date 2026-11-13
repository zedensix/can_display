# Reference wiring

This matches the pin constants at the top of `cmd/firmware/main.go`.
Change either the wiring or the constants if yours differs — they just
need to agree.

Board target: `pico-w` (Raspberry Pi Pico W / WH). GPIO23, 24, 25 and
29 are used by the wireless chip on the W, so don't wire anything to
them. Pin numbers below are GPn, not physical pin numbers.

## Display: ST7789 V2.2, 2.0" 240x320 SPI TFT

| Display pin | Pico W pin | Notes |
|---|---|---|
| SCK  | GP18 | SPI0 clock |
| SDA/MOSI | GP19 | SPI0 data out |
| CS   | GP17 | |
| DC   | GP20 | |
| RES/RST | GP21 | |
| BLK  | GP22 | backlight; driven high in software |
| VCC  | 3V3(OUT) | most of these panels are 3.3V logic *and* power, double check yours |
| GND  | GND | |

No MISO connection is used, the display is write-only.

## Car data, option A (default): ELM327 Bluetooth LE OBD dongle

Selected by `source = sourceELM327BLE` in `cmd/firmware/main.go`.
Nothing to wire: plug the dongle into the OBD port, the Pico W connects
to it over BLE with its onboard radio. Tested to compile against a
Vgate iCar Pro 2S, **not yet tested on the car**.

- The dongle must speak **BLE**, not only Bluetooth Classic: TinyGo has
  no Classic stack. A dongle that works with iPhones does BLE.
- Only one client at a time. If your phone (Car Scanner...) is
  connected to it, the Pico won't find it: turn the phone's Bluetooth
  off.
- What you get: RPM, coolant, AFR (standard OBD PIDs) and battery
  voltage (the dongle's `ATRV`). **No oil temperature**: it isn't a
  standard PID.
- Slower than the CAN option: every value is a request/answer through
  the dongle, a few refreshes per second at best.
- On boot the firmware prints what it does on the USB serial console
  (`tinygo monitor`): BLE candidates found, the dongle's GATT services.
  If it doesn't find yours, set `elmBLEAddress`, or add its service to
  `elmProfiles` in `cmd/firmware/elm_ble.go`.
- Any error (dongle unplugged, car off) reboots the Pico, which then
  scans again.

## Car data, option B: MCP2515 SPI CAN controller module

Selected by `source = sourceMCP2515`. Wired straight to the CAN bus:
faster, and gets oil temperature, but needs the wiring below.

| MCP2515 pin | Pico W pin | Notes |
|---|---|---|
| SCK  | GP10 | SPI1 clock |
| SI (MOSI) | GP11 | |
| SO (MISO) | GP12 | see the 5V warning below |
| CS   | GP13 | |
| INT  | *(unused)* | firmware polls instead of using the interrupt pin |
| VCC  | VBUS (5V) | the TJA1050 transceiver on most boards needs 5V |
| GND  | GND | |
| CAN-H / CAN-L | car's OBD-II port, pins 6 and 14 | see below |

**The RP2040 is not 5V tolerant.** On the common blue MCP2515 +
TJA1050 board, powering it from 5V makes SO swing to 5V, straight into
GP12. Either put a level shifter (or a simple divider) on SO, or do the
usual mod: feed the MCP2515 chip itself from 3V3 and keep only the
TJA1050 on 5V. The Pico's 3.3V outputs on SCK/SI/CS are read fine by a
5V MCP2515.

The firmware assumes a **16MHz crystal** on the MCP2515 board (the
common default on cheap breakout boards) and **500kbps**, which matches
the GT86's factory high-speed CAN bus. If your board uses an 8MHz
crystal, change `mcp2515.Clock16MHz` to `mcp2515.Clock8MHz` in
`setupCAN()`.

## Ethanol content: analog flex-fuel sensor, via the Pico ADC

The RP2040 has a 12-bit ADC on GP26-28, read through `machine.ADC`.

**The sensor outputs 0.5V-4.5V. That exceeds what the RP2040 can safely
see on an analog input, do not wire it in directly.** A resistor
divider brings it down first:

```
sensor signal ──[ R1: 10kΩ ]──┬── GP26 (ADC0)
                                │
                          [ R2: 22kΩ ]
                                │
                               AGND
```

This divider (ratio 22/32 ≈ 0.6875) brings the sensor's worst-case 4.5V
down to ~3.09V at the ADC pin, safely under the 3.3V rail, with the
math in `internal/sensors` (and a test asserting it stays under the
rail voltage, so a future edit can't silently make this unsafe).

The ADC reference (ADC_VREF) is the 3.3V rail on a stock Pico W, and
it's noisy. Calibrate `adcVref` in `cmd/firmware/main.go` against a
multimeter if the ethanol reading looks off; tying ADC_VREF to an
external 3.0V reference is the proper fix if it's still bad.

If you're using a different sensor with a different voltage range (or
find out this one's endpoints/direction are different from the
assumed 0.5V=0%/4.5V=100%), the two things to change are the divider
resistor values (`sensors.EthanolDivider`) and the sensor curve
(`sensors.EthanolPercent`), both isolated in `internal/sensors` with
their own unit tests in `tests/`.

## Connecting to the car

The GT86/BRZ/FR-S OBD-II port carries CAN H/L on the standard pins:

- Pin 6: CAN H
- Pin 14: CAN L

Do **not** connect the MCP2515's ground to anything other than a real
chassis/OBD ground, and don't power the Pico W from the car's 12V rail
without a proper automotive-grade buck converter (a car's electrical
system has voltage spikes well outside what a USB power path is rated
for). A cheap USB car charger or a dedicated 12V→5V buck module is the
easy, safe option for prototyping on the bench before it goes anywhere
near the dash.
