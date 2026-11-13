# can_display

A DIY CAN bus gauge cluster for a 2015 Toyota GT86, built in TinyGo for
a Raspberry Pi Pico W(H) + SPI TFT display.

The project is split so the dashboard UI can be built and iterated on
without a car, a Pico, or any wiring at all:

`cmd/simulator` and `cmd/firmware` both render the _exact same_
`internal/ui.Dashboard` — the simulator is a true preview, not a
lookalike. Only pin wiring and how frames arrive (real MCP2515 vs. a
fake generator) differ between the two.

## Quick start

```sh
mise run sim         # opens a desktop window with the live dashboard, fake data
mise run vet         # go vet the shared/simulator code + tinygo-build-check the firmware
mise run test        # unit tests for the hardware-agnostic packages
mise run firmware    # cross-compile firmware to bin/firmware.uf2
mise run flash       # build + flash, step by step; reboots a running Pico into BOOTSEL itself
mise run flash-sim   # same, with fake data instead of the car, to test the display
```

Go and [TinyGo](https://tinygo.org) are pinned in `mise.toml`: run
`mise install` once.

## Hardware this targets

- Raspberry Pi Pico W / WH (RP2040)
- ST7789 V2.2 2.0" 240x320 SPI TFT display
- MCP2515 SPI CAN controller module, wired to the OBD-II port's CAN H/L
  (pins 6/14)
- An analog (0.5V-4.5V) flex-fuel sensor for ethanol %, read through
  a resistor divider on the Pico's own ADC (see `docs/wiring.md`)

Full pin mapping: [`docs/wiring.md`](docs/wiring.md).

## What actually works today

| Signal          | Source                                                           | Status                                                                                                                                                                                                                               |
| --------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| RPM             | Factory CAN, `0x140`                                             | Decoded, **unverified on this car**                                                                                                                                                                                                  |
| Coolant temp    | Factory CAN, `0x360`                                             | Decoded, **unverified on this car**                                                                                                                                                                                                  |
| Oil temp        | Factory CAN, `0x360`                                             | Decoded, **unverified on this car**                                                                                                                                                                                                  |
| Ethanol %       | Pico ADC (flex-fuel sensor, not CAN)                             | Decoded, **sensor curve (0.5V=0%/4.5V=100%) unverified against the actual sensor's datasheet**                                                                                                                                       |
| AFR             | OBD-II Mode 01 PID 0x24, polled from the factory wideband sensor | Decoded, **speculative**: the PID/formula are a real SAE J1979 standard, but whether this ECU replies to it over CAN hasn't been confirmed, and even if it does, forum reports say the factory sensor can't show richer than ~12.2:1 |
| Battery voltage | —                                                                | Not found on the factory bus yet; tile is wired up and waiting                                                                                                                                                                       |

The CAN IDs/formulas in use come from community reverse-engineering,
not factory documentation — **do not trust them blindly**. See
[`docs/can-reverse-engineering.md`](docs/can-reverse-engineering.md)
for how to verify them on your own car (and how to add the missing
ones) with a cheap USB-CAN adapter. AFR is a different kind of
unverified — see `internal/canbus/gt86/afr.go`'s doc comment.

The simulator defaults to previewing all six tiles, including battery
(no data source at all yet), so the layout can be designed before that
hardware exists. Flip `Enable*` off in `cmd/simulator/main.go`'s
`main()` to see exactly what the firmware can show today.

## Design notes

- **`internal/display.Display`** is a type alias for
  `tinygo.org/x/drivers.Displayer`. Any TinyGo display driver satisfies
  it for free, and the simulator's software framebuffer satisfies it
  too — so `internal/ui` never needs to know which one it's talking to.
- **`internal/sensors`** holds conversion math for locally-wired analog
  sensors (currently just ethanol %) — the non-CAN counterpart to
  `internal/canbus/gt86`. Pure functions, no hardware access, so the
  voltage-divider and sensor-curve math is unit tested without needing
  a board or a multimeter.
- **Text rendering** uses `tinygo.org/x/tinyfont` (`freesans`) on both
  targets, on purpose — the simulator should look pixel-for-pixel like
  the real panel, not just "similar."
- **`internal/canbus.Registry`** dispatches frames to decoders by ID.
  The firmware feeds it real MCP2515 frames; a future `internal/sim`
  addition could replay a `candump` log through the same interface for
  regression testing against real captures.
- **`internal/canbus/obd2`** implements the request/response half of
  CAN (ISO 15765-4 Mode 01 PIDs), for signals like AFR that the ECU
  only sends if asked, as opposed to `gt86`'s passively-broadcast IDs.
  A `canbus.Writer` (separate from the read-only `canbus.Bus`, since
  most sources here can't transmit) is what makes sending a request
  possible in the first place.
- **`internal/ui/config.go`** is the one file to edit to retune what
  counts as normal/warning/danger for any gauge (idle RPM redline,
  coolant/oil temp limits, etc.) — `dashboard.go` just wires those
  values into tiles and shouldn't need to change alongside them.
- Tests live in `tests/` as an external package, so they only see
  exported API.
- Every package under `internal/` builds with plain `go build`/`go
test` — only `cmd/firmware` needs TinyGo (it's gated behind a
  `//go:build tinygo` tag so `go build ./...` from the repo root
  doesn't try and fail to compile it).
