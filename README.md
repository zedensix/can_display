<div align="center">

# GT86 Dash

**A gauge cluster for the Toyota GT86, built from a Raspberry Pi Pico W and a 2" screen.**

Oil, coolant, intake, AFR, battery and ethanol at a glance, in the GT86's own red-orange,
a race-wheel shift light across the top, and every drive logged to your phone.

[![CI](https://github.com/zedensix/can_display/actions/workflows/ci.yml/badge.svg)](https://github.com/zedensix/can_display/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-ff5a1e.svg)](LICENSE)
![Raspberry Pi Pico W](https://img.shields.io/badge/Raspberry%20Pi-Pico%20W-c51a4a.svg)
![C](https://img.shields.io/badge/firmware-C-555.svg)

<img src="ui/showcase.png" alt="The dash's screens: every gauge, the oil temperature alone as a dial, an alarm, the track layout, the settings, the screensaver" width="100%">

</div>

## Highlights

- **Reads the CAN bus itself.** An MCP2515 CAN module on the OBD port's CAN pins listens to
  what the car broadcasts many times a second and asks the engine ECU for the rest.
- **Looks like it came with the car.** Black, red-orange around the readings, segment gauges
  colored by band: green fine, amber warns, a reading in danger blinks its whole tile red,
  blue is an engine not warm yet.
- **Knows when you're on track.** With the traction or stability control off, the gauges
  switch to a track layout, the oil and the AFR big, under a badge for each mode past comfort
  (everything on): DRIFT (TRC off), SPORT (VSC sport), TRACK (both), RACE (everything off),
  read from bits two listens of the CAN bus agreed on.
- **Says what matters.** From 130°C the oil temperature takes the whole screen. The AFR stays
  calm off the throttle, when the fuel cut reads lean. A reading too old shows `--`, never a
  frozen number.
- **Reads what the ECU doesn't publish.** The oil temperature through Toyota's Mode 21, the
  ethanol % of a flex-fuel tune straight from the engine ECU's RAM.
- **Logs what checks the tune.** Besides the gauges: pedal, throttle, load, timing, both fuel
  trims, the AFR the ECU commands beside the one it gets, the direct injection rail's pressure
  and the speed, five times a second to its own flash; "Send log" hands them to a
  phone, and a small self-hosted site keeps every session and charts it.
- **Waits for the car.** Powered before the ignition wakes the bus, it says so and keeps
  listening; left waiting, it shows a screensaver of the badge and the logos of the parts on
  the car.

## The screens

| | | |
|:-:|:-:|:-:|
| <img src="ui/grid.png" width="240" alt="Every gauge"> | <img src="ui/oil.png" width="240" alt="A gauge alone"> | <img src="ui/alarms.png" width="240" alt="Alarms"> |
| **Every gauge**: the oil first, the rest as a list | **One gauge alone**, its session's min and max | **Alarms**: the row blinks red |
| <img src="ui/cold.png" width="240" alt="Cold engine"> | <img src="ui/shift.png" width="240" alt="Shift light"> | <img src="ui/oil_critical.png" width="240" alt="Oil overheating"> |
| **Cold engine**: blue, don't rev it yet | **Shift light**, lit in pairs from both ends | **Oil at 130°C**: the whole screen |
| <img src="ui/can_silent.png" width="240" alt="Waiting for the car"> | <img src="ui/settings.png" width="240" alt="Settings"> | <img src="ui/showcar.png" width="240" alt="Screensaver"> |
| **Waiting for the car**: the bus asleep | **Settings**, one screen, on the knob | **Screensaver**, nothing coming in |
| <img src="ui/track_trc.png" width="240" alt="Drift mode"> | <img src="ui/track_vsc.png" width="240" alt="Race mode"> | <img src="ui/shot_boot_morph.png" width="240" alt="Starting up"> |
| **Drift mode** (TRC off): the track layout | **Race mode**: everything off, in red | **Starting up**: the ring closes into the logo |
| <img src="ui/pedal.png" width="240" alt="Pedal"> | <img src="ui/load.png" width="240" alt="Engine load"> | <img src="ui/limiter.png" width="240" alt="Near the limiter"> |
| **Pedal**, a view on the knob | **Engine load**, another | **Near the limiter**: the whole screen flashes |
| <img src="ui/grid_landscape.png" width="240" alt="Landscape"> | <img src="ui/oil_landscape.png" width="240" alt="A gauge alone, landscape"> | <img src="ui/send_log_landscape.png" width="240" alt="Send log, landscape"> |
| **Landscape**, the panel on its side | **A gauge alone**, its extremes beside it | Every screen turns with it |
| <img src="ui/door.png" width="240" alt="Door open"> | <img src="ui/doors.png" width="240" alt="Doors open"> | <img src="ui/can_failed.png" width="240" alt="CAN self test failed"> |
| **A door open**: which, on the car | **Several**: each swung out | **CAN self test**: which wire to check |

Along the bottom of the gauges: the gear engaged (worked out from the RPM and the speed), the
temperature outside and the distance driven. A rotary encoder drives it all: turn for the
views, click for the settings, hold to reboot. The screen turns a quarter turn in the
settings, for a panel mounted on its side.
Every picture here comes from the UI code itself (`mise run ui`), the same code the firmware
runs.

## Every drive, logged

<img src="docs/resources/web-session.png" alt="A session's charts in the log site, dark theme" width="100%">

"Send log" turns the dash into a Bluetooth peripheral, the gauges paused meanwhile; the log
page, opened in Chrome on Android, downloads the whole log and sends it to its server. The server keeps each session
once, however many times it's sent, even after the dash's flash ring has overwritten it.
The list pages, searches and filters by date; a session can be named, and gets its peaks,
a chart per signal (throttle included), a cursor across them all, and a CSV. Light, dark or
the device's theme.

<img src="docs/resources/web-logs.png" alt="Every stored session, light theme" width="100%">

The page self-hosts with the `Dockerfile` (port 3000, the logs in its `/data` volume), behind
an https reverse proxy: Web Bluetooth needs https, and the proxy is what asks who's there.

## Hardware

| Part | For | Notes |
| --- | --- | --- |
| Raspberry Pi Pico W (or WH) | the brain, and Bluetooth for Send log | RP2040, 2MB flash |
| ST7789 V2.2 2.0" 240x320 SPI display | the screen | in portrait |
| KY-040 rotary encoder (EC11) | turn, click, hold | wired on its own legs, the Pico's pull-ups |
| MCP2515 + TJA1050 CAN module | the car's data | on OBD pins 6 and 14, its SO through a resistor divider (5V to 3.3V) |
| 12V to 5V converter | power | from a switched 12V, into the Pico's USB socket |
| Printed faceplate and tray | holding it all | modeled in code, see below |

The full pin map and power: [docs/wiring.md](docs/wiring.md).

<img src="docs/resources/wiring-color.png" alt="The wiring diagram" width="100%">

`bun run wiring` opens the diagram in a browser: zoom, follow a wire, turn a board over.

## The enclosure

<p>
  <img src="faceplate/plate/renders/assembly-front.png" width="32%" alt="The assembly from the front">
  <img src="faceplate/plate/renders/assembly.png" width="32%" alt="The assembly from the back">
  <img src="faceplate/plate/renders/assembly-side.png" width="32%" alt="The assembly from the side">
</p>

A faceplate holds the display and the encoder; a tray behind it carries the Pico, the CAN
board and the converter. Both are modeled in TypeScript (scad-js) from the modules' measured
dimensions, print without supports, and come out as STLs with `mise run faceplate`. The
wiring page's Dimensions tab is where the measurements go in. More in
[faceplate/README.md](faceplate/README.md).

## Getting started

1. Install [mise](https://mise.jdx.dev), then `mise install`: the ARM toolchain, CMake, Bun
   and the rest, pinned, and the git hooks.
2. `mise run sim` opens the dash in a window, with made-up data: the arrows turn the knob,
   space clicks, l turns it to landscape (the simulator needs SDL2: `brew install sdl2`).
3. `mise run firmware` builds `bin/firmware.uf2`; `mise run flash` builds and flashes a Pico
   on USB, rebooting it into BOOTSEL by itself.
4. In the car: wire the MCP2515 to the OBD port, power the dash from a switched 12V.

## Development

```sh
mise run sim          # the dash in a window, made-up data
mise run test         # host tests of the UI and data code, with ASan and UBSan
mise run firmware     # bin/firmware.uf2, and bin/firmware-sim.uf2 with made-up data
mise run flash        # build and flash a Pico on USB
mise run logs         # the log the Pico keeps in flash: boots, the CAN self test, Send log, crashes
mise run ui           # every screen as a picture in ui/, and the showcase above
mise run faceplate    # the printed parts: STLs and renders
mise run logo         # the boot splash, from 86.png
mise run logos [dir]  # the screensaver's parts' logos, from your own folder (kept out of git)
bun run dev           # the log site on localhost:3000
bun test web          # the log site's store
bun run wiring        # the wiring diagram and the modules' dimensions
mise run wiring       # its pictures, for docs/wiring.md
mise run web-pictures # the log site's pictures, from made-up drives
```

The CI runs every hook (clang-format, Biome, `bun check` for the types, the tests) and builds the firmware.

## What works today

| Signal          | Source                                              | Status |
| --------------- | --------------------------------------------------- | ------ |
| RPM (shift light) | Broadcast `0x140`                                 | **Unverified on this car** |
| Coolant temp    | Broadcast `0x360`                                   | **Unverified on this car** |
| Oil temp        | Broadcast `0x360`                                   | **Unverified**; Toyota's Mode 21 PID 01, checked on this car against another gauge, isn't asked |
| Intake air temp | OBD PID 0x0F                                        | PID is standard; the ECU lists it as supported |
| AFR             | OBD PID 0x24 (wide-range O2 sensor 1)               | The ECU lists the PID as supported; values **not cross-checked** |
| Throttle        | OBD PID 0x11                                        | Read on this car; tells a lean AFR off the throttle from a real one |
| Battery voltage | OBD PID 0x42, the engine ECU's supply               | A few tenths under the battery's |
| Ethanol %       | The engine ECU's RAM at 0xFFF87C7C                  | Found by scans and matched against OFT on this car; specific to this tune |
| Pedal, load, timing, fuel trims, commanded AFR, DI rail pressure, speed, outside temperature | OBD PIDs 0x5A, 0x04, 0x0E, 0x06/0x07, 0x44, 0x23, 0x0D, 0x46 | The ECU lists them all as supported; values **not read yet** |
| Gear | RPM over speed, the manual gearbox's ratios | Worked out, **not checked in the car** |
| Odometer | Not asked | The dash shows the trip since boot |

The CAN IDs and formulas come from community reverse-engineering, not factory documentation:
**don't trust them blindly**. [docs/can-reverse-engineering.md](docs/can-reverse-engineering.md)
covers how to check them on your own car, and how this one's were found.

## Design notes

- **One UI, three targets.** The firmware, the simulator and the tests all draw through the
  same code into an RGB565 canvas; the firmware pushes it to the panel with DMA. The simulator
  is a pixel-exact preview, so UI work happens there.
- **Two cores, one loop.** Core1 only reads the CAN bus into a queue, as fast as SPI goes;
  core0 decodes it, draws, and runs the Bluetooth stack for Send log between frames. No RTOS,
  no interrupt racing the UI.
- **Failures are visible.** The CAN self test says on the panel which wire to check. A watchdog reboots a frozen board, a panic is written to the flash log before
  rebooting, and a hard fault is reported at the next boot.
- **Thresholds live in one place**, the ranges at the top of `src/app/ui/ui.c`.

## License

MIT ([LICENSE](LICENSE)), except the font data in `src/app/ui/fonts.c`: FreeSans Bold from
GNU FreeFont, GPLv3 with the font exception. The 86 logo (`86.png`, `src/app/ui/logo.c`) is
Toyota's trademark, not covered by the MIT license. The screensaver's other logos, the car's
parts' makers', aren't in the repository: `mise run logos` builds them in from your own
folder.
