# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

DIY gauge cluster for a 2015 Toyota GT86: C firmware (pico-sdk 2.2, BTstack) for a Raspberry Pi Pico W(H) + ST7789 V2.2 2" 240x320 SPI display, getting car data from an ELM327 BLE OBD dongle (default) or an MCP2515 CAN controller, plus an SDL desktop simulator and host tests that run the same UI code.

## Commands

```sh
mise run sim         # SDL simulator, fake data; arrows turn the encoder, space clicks
mise run test        # host tests of src/app (test/tests.c), ASan + UBSan
mise run firmware    # bin/firmware.uf2 + bin/firmware-sim.uf2; clones the pico-sdk into PICO_SDK_PATH the first time
mise run flash       # build, reboot a running Pico into BOOTSEL (1200 baud touch), dd the UF2; prints each step
mise run flash-sim   # same with the fake-data firmware (SIM=1)
mise run logs        # print the flash log (sends 'l' over USB serial)
mise run logo        # 86.png -> src/app/logo.c (tools/logo.py, then xxd)
mise run ui          # every screen with fixed readings -> ui/*.png, ui/sheet.png (tools/render_ui.c): rerun after a UI change and look at it
```

The ARM toolchain (14.3.Rel1), CMake, Ninja, Python and the SDK version/path are pinned in `mise.toml`. Our sources build with `-Wall -Wextra -Werror` on every target (SDK sources excepted): a warning is a failed build, there's no separate lint.

This project started in Go/TinyGo and was rewritten in C (see git history before `cbb43b4`): TinyGo's cooperative scheduler, 8KB goroutine stacks, fonts in RAM and its USB and BLE stacks kept breaking on the Pico W.

## Architecture

Data flows one way: **source -> `state_t` -> `ui_render(canvas, state, screen, settings, now)` -> panel**.

- `src/app` is hardware-free C shared by the firmware, the simulator (`sim/`) and the tests (`test/`): signals, OBD/GT86 decoding, the ELM327 session, sensors math, settings encoding, the flash log ring, encoder navigation, gfx and the UI. Anything testable goes there.
- `src/firmware` is the Pico side: `board.h` (pins, `DATA_SOURCE`), `display.c` (ST7789 init, PWM backlight, flip via MADCTL, async DMA push of 16-bit frames), `knob.c` (encoder on GPIO IRQ with a quadrature table, detents counted at rest so a lost step can't shift them; button debounced, a click acts on press; the loop redraws at once on input via `knob_pending`), `store.c` (flash log, settings, serial dump), `ble.c`, `mcp2515.c`, `main.c`.
- Single loop, no RTOS: `pico_cyw43_arch_poll`, so BTstack runs only when `ble_poll` calls `cyw43_arch_poll()`: between frames, and while the DMA pushes one. Nothing may block the loop.
- `ble.c` is a phase machine: prompt -> start (cyw43 + BTstack) -> scan (30s timeout, matches name hints or `profiles` services, or `ELM_ADDRESS`) -> connect -> GATT discovery (logs every service/characteristic) -> notifications -> `elm_t` session. Writes are "without response"; a command the stack can't take yet stays pending for the next poll.
- `elm327.c` is a non-blocking ELM327 session: init script (ATZ, echo/spaces off, headers on, ATSP6, ATSH7DF), probes whose raw answers are logged (ATI, ATRV, 0100/0120/0140 supported-PID bitmaps, 010C, and `010C24050F1`: if its answer holds all four PIDs, polling asks for them in one request), then a poll cycle (that one request, or RPM every other request between AFR, coolant and intake one at a time; ATRV every second, and every 3s the oil temperature: ATSH7E0, 2101, ATSH7DF, dropped after 3 answers without it while the ECU answers the rest). Multi-frame answers are reassembled by `isotp_feed`. Timeouts resend `ELM_RETRIES` times. With headers on each reply line is a CAN frame, decoded by `obd_decode` (ECU1 0x7E8, 11bit IDs only). The "1" response-count suffix is dropped if the dongle answers `?`. "Scan ECU RAM" in the settings (`elm_scan_ram`) suspends polling to read the engine ECU's RAM and log ethanol % candidates (see below), with its progress as the status. "Listen CAN" (`elm_listen_can`, `sniff.c`) finds a button's CAN ID by elimination: the dongle monitors one window of 256 IDs at a time (ATCAF0, ATCM700, ATCF, ATMA, frames fed line by line as they stream), 45s hands off marks every changing ID as noise, then 60s of presses; the candidates are logged and the session restarts with ATZ. The sport mode's ID is what it's for; it isn't known yet.
- Connection control: without auto-connect the source waits on "press to connect"; a failure waits for a click to retry (auto-connect: retries after 5s). `main.c` hands a click to `ble_prompt_click` only on the gauges page. A retry is `watchdog_reboot` with `CONNECT_NOW` in watchdog scratch 0, so the next boot connects without asking.
- Standby: once polling, if the engine ECU hasn't answered anything for `CAR_OFF_MS` (90s), `ble.c` disconnects (so the dongle, always powered by the OBD port, can sleep) and powers the radio off; `main.c` puts the panel to sleep and stops drawing. A click reboots to connect. It saves what software can: a Pico on an always-live supply still draws ~20mA.
- `screen_t` is what the encoder drives: on the gauges, turning cycles views (grid, then each gauge alone, its value drawn 2-3x by `gfx_text`'s scale instead of a bigger font); a click opens the settings page (brightness, shift light RPM, auto-connect, flip, ethanol sensor: the ADC is only read when on, an unconnected pin floats; Scan ECU RAM, Listen CAN), which `main.c` applies at once and saves to flash when leaving the page. A 1s hold reboots.
- `state_t.status` is the source's report while it isn't delivering: while non-empty, the gauges page draws a status screen (title, wrapped lines, spinner) instead of the tiles. A reading older than its tile's `stale_ms` renders as "--" (2s, 10s for the oil temperature polled every 3s). At power-on `ui_render_boot` reveals the logo behind a scan line, revs the shift light to a blue flash, then sweeps a GT86-style tachometer to its end and back.
- Thresholds are the `*_RANGE` macros at the top of `src/app/ui.c`; `ui_tiles` wires labels, formats and units onto them.
- The look copies the GT86's own cluster: black, red-orange around the readings (`accent`), white labels; readings take their band's color: green fine, amber warns, a reading in danger blinks its whole tile red, blue is an engine not warm yet (oil under 80°C, coolant under 70°C: not alarms, every cold start goes through them). From `OIL_CRITICAL` (130°C) the oil temperature takes the whole screen, on every page. Gauges are segments colored by the band each stands for, the unlit ones faintly so the red zone shows at rest; a gauge alone is a round dial like the tachometer, with the session's min and max (`reading_t`, since boot) and its peak marked. No RPM tile (the car's tachometer is in front of the driver): RPM drives the shift light, a race wheel's LED bar across the top of the gauges, 20 narrow LEDs lighting in pairs from both ends over the last 1500 rpm (green, amber, red), all flashing blue from the shift RPM.

Adding a signal: a `signal_t` value, a decoder case in `obd.c` (or a sensor function plus a read in `main.c`), a tile + range in `ui.c`, a poll entry in `elm327.c` if it's a PID, `simdata.c`, and tests.

## Conventions and gotchas

- CAN IDs and formulas come from community reverse-engineering and are **unverified on this car**. Verified on this car so far: the oil temp via Mode 21 PID 01 (byte AC - 40), and that the ECU lists PID 0x24 (AFR) as supported, not 0x52 (ethanol) nor 0x5C (oil). On its memory: Subaru SSM reads (A0) get no answer; UDS ReadMemoryByAddress (`23 14` + 4 address bytes + 1 size byte, header 7E0) reads RAM from 0xFFF80000 without security access but answers 7F 23 33 (security access denied) on ROM. Where the ethanol % sits in RAM isn't found yet. In `ble.c`, only the 18F0 GATT profile is confirmed (on the Vgate iCar Pro 2S); the others are guesses. Don't present any of them as confirmed. `docs/can-reverse-engineering.md` covers how to verify them.
- RAM: the full frame (153.6KB) is a static array, BTstack and cyw43 use static pools: overflow is a link error, not a runtime surprise. If a change doesn't link, `canvas_t` supports drawing in bands (draw and push per band).
- Flash, from the end of the 2MB: BTstack's own two sectors (`pico_btstack_flash_bank`), the 64KB log ring, then the settings sector. `store_init` refuses to touch flash if the firmware ever reaches the settings sector. Flash writes go through `flash_safe_execute` (interrupts off): `log_line` is for events only, never per frame or per OBD reply.
- Crashes leave a trace: the watchdog (8s, fed once per loop) reboots a frozen board and the next boot logs it; `PICO_PANIC_FUNCTION=crash_panic` writes the panic message to the flash log; `isr_hardfault` marks watchdog scratch 1 and the next boot logs it.
- USB serial is TinyUSB (`pico_stdio_usb`): the 1200 baud touch reboots into BOOTSEL, which `mise run flash` relies on.
- Hardware facts that constrain the code (see `docs/wiring.md`): the RP2040 is not 5V tolerant (MCP2515 SO line needs level shifting, the KY-040 goes on 3V3). GPIO23/24/25/29 belong to the wireless chip, GP15 to the SDK's RP2040-E5 USB workaround. The flex-fuel sensor outputs 0.5-4.5V, so it goes through `ETHANOL_DIVIDER` (a test asserts the divided max stays under 3.3V). `ADC_VREF` is a calibration knob.
- Pin constants in `src/firmware/board.h` and the tables in `docs/wiring.md` must stay in sync. Same for the panel size in `board.h` and `sim/main.c`.
