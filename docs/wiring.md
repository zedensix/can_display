# Reference wiring

This matches the pin constants in `src/firmware/board.h`.
Change either the wiring or the constants if yours differs: they just
need to agree.

Board target: `pico-w` (Raspberry Pi Pico W / WH). GPIO23, 24, 25 and
29 are used by the wireless chip on the W, so don't wire anything to
them. Pin numbers below are GPn, not physical pin numbers.

## Display: ST7789 V2.2, 2.0" 240x320 SPI TFT

| Display pin | Pico W pin | Notes |
|---|---|---|
| SCL | GP18 | SPI0 clock (SCK), despite the I2C-style name |
| SDA | GP19 | SPI0 data out (MOSI), despite the I2C-style name |
| CS | GP17 | |
| DC | GP20 | |
| RES | GP21 | |
| BL | GP22 | backlight; driven high in software |
| VDD | 3V3(OUT) | most of these panels are 3.3V logic *and* power, double check yours |
| GND | GND | |

It's SPI, not I2C: no MISO, the display is write-only.

## Rotary encoder: KY-040 (EC11)

| KY-040 pin | Pico W pin | Notes |
|---|---|---|
| CLK | GP2 | quadrature A, read on interrupts |
| DT | GP3 | quadrature B |
| SW | GP4 | push button, active low; the Pico's internal pull-up is used |
| + | 3V3(OUT) | **not 5V**: the board's pull-ups go to +, and the RP2040 isn't 5V tolerant |
| GND | GND | |

Turn on the gauges: every gauge, then each one alone, big. Click on the
gauges: settings page (turn to pick a row, click to change it, Back to
leave); while the screen says "press to connect" or "press to retry",
the click goes to the connection instead. Hold 1s: reboot. If turning
goes the wrong way, swap CLK and DT. `STEPS_PER_DETENT` in
`src/firmware/knob.c` is 2, for a half-step encoder (the one on this
car); if one click moves two rows, it's a full-step one: set it to 4.

Keep GP15 free: the pico-sdk drives it briefly at boot for the RP2040 USB
enumeration workaround (erratum RP2040-E5).

## Power

On the bench, USB is enough. In the car, the Pico W needs 5V from the
car's 12V through a **12V to 5V buck converter**. Never wire 12V to the
Pico: the car's electrical system has spikes far beyond what it takes.
A USB car charger, or an automotive buck module rated 1A or more, does
the job.

| From | To | Notes |
|---|---|---|
| switched 12V (fuse tap on an ACC/ignition fuse) | buck IN+ | see below |
| chassis ground | buck IN- | |
| buck OUT+ (5V) | VSYS (physical pin 39) | accepts 1.8V to 5.5V |
| buck OUT- | GND | |

- **Take a switched 12V, not OBD pin 16.** Pin 16 is live with the car
  off: the Pico would keep scanning for the dongle and drain the
  battery overnight.
- With the Pico on VSYS, **VBUS stays dead** unless USB is plugged in.
  Anything needing 5V (the MCP2515 board, a 5V sensor) takes it from
  the buck output, not from VBUS.
- To plug in USB (flashing, `mise run logs`) while the car feeds VSYS,
  put a Schottky diode in series between buck OUT+ and VSYS, so the two
  supplies don't fight. Otherwise unplug one of them.

## Car data, option A (default): ELM327 Bluetooth LE OBD dongle

Selected by `DATA_SOURCE` in `src/firmware/board.h`.
Nothing to wire: plug the dongle into the OBD port, the Pico W connects
to it over BLE with its onboard radio. Written for a Vgate iCar Pro 2S,
seen working on hardware: it advertises no name, and serves its serial
port on service `18f0` (notify `2af0`, write `2af1`). It answers `ATI`
with `ELM327 v2.3`. The car side (PIDs answering, values) isn't
verified yet.

- The dongle must speak **BLE**, not only Bluetooth Classic: the
  firmware only does BLE. A dongle that works with iPhones does.
- Only one client at a time. If your phone (Car Scanner...) is
  connected to it, the Pico won't find it: turn the phone's Bluetooth
  off.
- What you get: RPM, coolant, AFR (standard OBD PIDs), battery
  voltage (the dongle's `ATRV`), and oil temperature through Toyota's
  Mode 21 PID 01 asked to the engine ECU (community-sourced, checked
  on this car against another gauge). Ethanol % isn't available over
  standard OBD: the ECU doesn't support PID 0x52.
- Slower than the CAN option: every value is a request/answer through
  the dongle, a few refreshes per second at best.
- No pairing: these dongles take any BLE client without a PIN. The
  Pico connects to the first device that looks like one; set
  `ELM_ADDRESS` in `src/firmware/ble.c` to lock it to yours.
- Until data flows, the panel shows a status screen: scanning (with
  the last device it ignored), connecting to (name and MAC), reading
  services, init. The flash log (`mise run logs`) also lists the
  dongle's GATT services and its raw answers to a few probes.
- If the scan never stops on your dongle, its name and services match
  nothing known: read them with a phone BLE scanner (nRF Connect), then
  set `ELM_ADDRESS` or add its service to `profiles` (both in
  `src/firmware/ble.c`). The scan gives up after 30s.
- The connection starts with a click on the encoder ("press to
  connect"), or at power-on with Auto-connect on in the settings. Any
  error (dongle unplugged, car off) shows on the panel and waits for a
  click to retry; with Auto-connect, it retries by itself after 5s.

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
| VCC  | 5V | the TJA1050 transceiver on most boards needs 5V: VBUS on USB, the buck output in the car (see Power) |
| GND  | GND | |
| CAN-H | OBD-II pin 6 | |
| CAN-L | OBD-II pin 14 | |

**The RP2040 is not 5V tolerant.** On the common blue MCP2515 +
TJA1050 board, powering it from 5V makes SO swing to 5V, straight into
GP12. Either put a level shifter (or a simple divider) on SO, or do the
usual mod: feed the MCP2515 chip itself from 3V3 and keep only the
TJA1050 on 5V. The Pico's 3.3V outputs on SCK/SI/CS are read fine by a
5V MCP2515.

**Remove jumper J1** on that board. It adds a 120Ω termination
resistor, but the car's bus is already terminated at both ends; a third
one on the OBD stub drags the bus impedance down and causes errors.

The firmware assumes a **16MHz crystal** on the MCP2515 board (the
common default on cheap breakout boards, but 8MHz ones exist: read the
marking on the can-shaped crystal) and **500kbps**, which matches the
GT86's factory high-speed CAN bus. For an 8MHz crystal, change
`mcp2515.Clock16MHz` to `mcp2515.Clock8MHz` in `setupCAN()`.

Ground the board only through the Pico's ground, which goes to the car
chassis via the buck converter. Never to anything else.

## Ethanol content: analog flex-fuel sensor, via the Pico ADC

The RP2040 has a 12-bit ADC on GP26-28. The firmware only reads it with
"Ethanol sensor" on in the settings: an unconnected pin floats and reads
as a plausible, drifting ethanol %.

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
math in `src/app/sensors.c` (and a test asserting it stays under the
rail voltage, so a future edit can't silently make this unsafe).

The ADC reference (ADC_VREF) is the 3.3V rail on a stock Pico W, and
it's noisy. Calibrate `ADC_VREF` in `src/firmware/board.h` against a
multimeter if the ethanol reading looks off; tying ADC_VREF to an
external 3.0V reference is the proper fix if it's still bad.

On this car the sensor already feeds the ECU's flex-fuel tune through a
0.5-4.5V signal: tap that same wire (signal and ground) into the divider.
The tune's "Ethanol Sensor Scaling" is 25%/V and -12.5%, i.e.
0.5V=0%/4.5V=100%, the curve the firmware uses: the dash and the ECU agree.

If you're using a different sensor with a different voltage range (or a
tune with another scaling), the two things to change are the divider
resistor values (`ETHANOL_DIVIDER`) and the sensor curve
(`sensors_ethanol_percent`), both in `src/app/sensors.c` with their own
tests in `test/tests.c`.
