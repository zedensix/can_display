# can_display

A DIY gauge cluster for a 2015 Toyota GT86: C firmware (pico-sdk) for a
Raspberry Pi Pico W(H) and a 2" 240x320 ST7789 display, getting car data
from an ELM327 Bluetooth LE OBD dongle (default) or an MCP2515 CAN
controller.

The UI code is plain C with no hardware access, so it also builds for a
desktop simulator and for host-side tests: the dashboard can be worked
on without a car, a Pico, or any wiring.

## Quick start

```sh
mise install         # the ARM toolchain, CMake, Ninja, Python (for the logo tool)
mise run sim         # desktop window with the live dashboard, fake data (arrows/space = the rotary encoder)
mise run test        # host-side tests, with the address and UB sanitizers
mise run firmware    # bin/firmware.uf2 and bin/firmware-sim.uf2 (fetches the pico-sdk once)
mise run flash       # build + flash, step by step; reboots a running Pico into BOOTSEL itself
mise run flash-sim   # same, with fake data instead of the car, to test the display
mise run logs        # read the log the Pico kept in flash (boots, BLE steps, dongle answers, crashes)
mise run logo        # regenerate the boot splash from 86.png
mise run ui          # render every screen into ui/*.png (ui/sheet.png: all side by side)
```

The simulator needs SDL2 (`brew install sdl2`).

## Hardware

- Raspberry Pi Pico W / WH (RP2040)
- ST7789 V2.2 2.0" 240x320 SPI TFT display
- A KY-040 rotary encoder: turn to change the gauge view, click for the
  settings (brightness, auto-connect, flip, ethanol sensor), hold 1s to
  reboot
- Car data, one of:
  - an ELM327 BLE OBD dongle (written for a Vgate iCar Pro 2S): no wiring
  - an MCP2515 SPI CAN module on the OBD-II port's CAN H/L (pins 6/14)
- Optionally an analog (0.5-4.5V) flex-fuel sensor for ethanol %, read
  through a resistor divider on the Pico's ADC

Full pin mapping and power: [`docs/wiring.md`](docs/wiring.md).

## What works today

| Signal          | Source                                              | Status                                                                                                                                                         |
| --------------- | --------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| RPM             | OBD PID 0x0C (dongle), or broadcast `0x140` (CAN)   | PID is standard; `0x140` **unverified on this car**                                                                                                            |
| Coolant temp    | OBD PID 0x05 (dongle), or broadcast `0x360` (CAN)   | PID is standard; `0x360` **unverified on this car**                                                                                                            |
| Oil temp        | Toyota Mode 21 PID 01 (dongle), or broadcast `0x360` (CAN) | Mode 21 checked on this car against another gauge; `0x360` **unverified** |
| AFR             | OBD PID 0x24 (wide-range O2 sensor 1)               | The ECU lists the PID as supported; values **not cross-checked**, and forum reports say the factory sensor can't show richer than ~12.2:1 |
| Battery voltage | The dongle's `ATRV`                                 | Read on hardware (dongle only)                                                                                                                                 |
| Ethanol %       | Pico ADC, on the flex-fuel sensor's signal         | Curve matches the tune's scaling (25%/V, -12.5%); needs the signal wire tapped and "Ethanol sensor" on. Reading it from the ECU's RAM instead is in progress (see the reverse-engineering doc) |

The CAN IDs and formulas come from community reverse-engineering, not
factory documentation: **don't trust them blindly**. See
[`docs/can-reverse-engineering.md`](docs/can-reverse-engineering.md) for
how to verify them on your own car.

## Design notes

- **One UI, three targets.** The firmware, the simulator and the tests
  all draw through the same code into an RGB565 canvas. The firmware
  pushes it to the panel with DMA; the simulator shows it in an SDL
  window. The simulator is a pixel-exact preview, so UI work happens
  there.
- **Fonts and the logo are generated C arrays in flash** (FreeSans Bold
  in Adafruit GFX format). The degree sign isn't in the font, so the
  text code draws it.
- **Single loop, no RTOS.** The BLE stack (BTstack, cyw43 "poll"
  architecture) runs from the main loop, between frames and while the
  DMA pushes one. No interrupt races the UI, and the ELM327 session is a
  non-blocking state machine with timeouts and retries.
- **Failures are visible.** The data source reports what it's doing on
  the panel (scanning, connecting, init) and what went wrong. A
  watchdog reboots a frozen board, a panic is written to the flash log
  before rebooting, and a hard fault is reported at the next boot.
- **Thresholds** (what counts as normal, warning, danger for each
  gauge) live in one place, the ranges at the top of `src/app/ui.c`.
