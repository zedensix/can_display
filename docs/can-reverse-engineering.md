# Verifying and extending the GT86 CAN decoders

## Why this matters

The IDs and formulas in `src/app/obd/obd.c` come from community
reverse-engineering (see the source link in that package's doc comment),
not from Toyota/Subaru factory documentation. They have **not** been
verified against this specific car. Treat every value as "probably
right, please confirm" before trusting anything the dash shows you —
especially before making driving decisions based on it.

## What's already wired up

| Signal | Status |
|---|---|
| RPM | Decoded from `0x140` bytes 2-3, 14-bit little-endian, no scaling. Unverified on this car. (`0x141` bytes 4-5 carries a second copy, per the same source, if this one needs cross-checking.) |
| Coolant temp | Decoded from `0x360` byte 3, `value - 40`°C. Unverified on this car. |
| Oil temp | Decoded from `0x360` byte 2, `value - 40`°C. Unverified on this car. |
| AFR | Polled via OBD-II Mode 01 PID 0x24 (standard, not reverse-engineered) from the factory wideband sensor. Whether this ECU actually answers it hasn't been confirmed — see the dedicated section below. |
| Battery voltage | **Not found** in any factory-bus reverse-engineering doc so far. Needs new reverse-engineering work (see below). |
| Ethanol % | Not on CAN nor a PID: read from the engine ECU's RAM (below). |

## How to check (or find) an ID

You don't need this project's hardware to do this — any cheap USB-CAN
adapter (e.g. an $8 "CANable"-clone, or even another MCP2515 board on a
breadboard talking to a laptop) plus `candump`/`cansniffer`
(linux `can-utils`) or [SavvyCAN](https://www.savvycan.com/) (cross
platform, nicer UI) works.

1. Connect to the OBD-II port, pins 6 (CAN H) and 14 (CAN L).
2. With the engine idling and everything electrically "boring" (no
   accessories being toggled), log a minute or two of traffic.
3. To find a specific signal: change *one thing* at a time and diff the
   log against a baseline.
   - RPM: this is the easiest one to sanity-check — rev the engine and
     watch bytes 2-3 of `0x140` count up and back down in step with the
     tach. If they don't, it's worth trying bytes 4-5 of `0x141`
     instead (the same value is supposedly duplicated there).
   - Coolant/oil temp: compare a cold start against 5 minutes of idling
     — the target bytes should climb steadily as the values in
     `0x360` are expected to (byte 2 and 3, `raw - 40` = °C). If they
     don't match on your car, `cansniffer` (which highlights bytes
     that change) makes this fast to spot.
   - Battery voltage: turn on/off a big electrical load (headlights,
     rear defroster) and watch for a byte that dips and recovers by
     roughly the right amount (idle ~14.2V, load ~13.5-13.8V).
4. Once you've found a candidate ID/byte/formula, add a decoder in
   `gt86_decode_broadcast` (`src/app/obd/obd.c`) following the pattern of the
   `GT86_ID_TEMPERATURES` case.
5. Update the status table above and the comment citing your source
   (even if the source is "captured myself on <date>, see <log file>")
   so the next person (including future you) knows what's actually been
   confirmed versus copied from a forum post.

## Verifying AFR (OBD-II PID 0x24)

This one's worth checking *before* wiring up the real hardware, since
it's easy to test with tools that already exist:

1. Get any ELM327-based OBD-II adapter (a $10 Bluetooth/USB one is
   fine) and a laptop.
2. Use [python-OBD](https://python-obd.readthedocs.io/) or a generic
   PID-request tool to send Mode 01 PID 0x24 and see if you get a
   response at all. python-OBD has this PID built in as
   `obd.commands.O2_S1_WR_VOLTAGE` / equivalence-ratio commands --
   easier than crafting raw CAN frames by hand for a first check.
3. If you get a response: note whether it came back on the standard
   `0x7E8` address or something else, and roughly how long it took --
   `AFR_INTERVAL_MS` in `src/firmware/hardware/mcp2515.c` assumes the ECU can
   keep up with a request every 250ms; slow it down if not. (Through
   the ELM327 dongle, requests go back to back, paced by its answers.)
4. If a functional-broadcast request (`0x7DF`, what this project sends
   by default) gets no response, try a *physical* request to `0x7E0`
   instead (the engine ECU's usual address) -- some ECUs are picky
   about which addressing mode they answer. That'd mean changing
   `OBD_REQUEST_ID` (`src/app/obd/obd.h`) to `0x7E0`, and `ATSH7DF` in
   `src/app/obd/elm327.c` to `ATSH7E0` for the dongle.
5. Sanity check the actual number: with the engine warm and idling,
   AFR should hover close to 14.7 (lambda ~1.0). If it's stuck at 0,
   pegged at max, or wildly implausible, something in the request
   format or response parsing doesn't match this ECU -- compare
   against what your OBD-II tool captured in step 2-3 byte-for-byte
   against `obd_build_request`/`obd_decode_frame`.

## Adding an aftermarket AFR source instead

The OBD-II route only gets you what the factory sensor's ECU-side
scaling allows (reportedly capped around 12.2:1 rich, per community
reports). If that turns out to be too
limiting, or PID 0x24 doesn't work on this car at all, a standalone
wideband controller broadcasting onto CAN (Haltech, AEM, etc. all have
documented CAN broadcast protocols) is the fallback: find its frame
layout and add a case for its ID in `gt86_decode_broadcast`, same pattern as
everything else there, no changes needed anywhere else in the pipeline.

## Reading the ethanol % from the ECU's RAM

The flex-fuel tune reads the sensor itself, so the ECU holds the
ethanol % somewhere in RAM: reading it there avoids tapping the sensor's
signal wire, which the injection depends on. Found on this car so far:

- Subaru SSM memory reads (`A0 00 <address> <count-1>` to 0x7E0): no
  answer at all.
- UDS ReadMemoryByAddress (`23 14 <4 address bytes> <size>` to 0x7E0):
  RAM reads work from 0xFFF80000 with no security access; ROM reads
  (0x11CC38, the tune's "Ethanol Sensor Scaling") answer `7F 23 33`,
  security access denied. `23 <3 bytes> <size>` (the KWP2000 layout)
  answers `7F 23 31`: the ECU parses UDS.
- An Alientech log of this ECU puts every ethanol % on
  25 x n x 5/4096 - 12.5: a 12-bit ADC on 5V through the tune's scaling.

"Scan ECU RAM" on the settings page scans it (`ELM_SCAN_*` in
`src/app/obd/elm327.h`, 2 to 3 minutes, polling paused) and logs every float on that grid, as an ethanol %
or as the sensor's voltage. To pin the address down, compare the
candidates with what OFT or another logger shows at the same time, then
across two tanks with different blends: the right address follows the
blend.

Three scans on one tank (2026-11-15 and 16, engine running) found a
single address every time: **0xFFF87C7C**, at 66.66, 65.72 and 66.79 %
(the third scan also had 66.785 at 0xFFF8D410, a copy), and it matched
OFT's reading. The dash polls it (`ETHANOL_RAM_READ` in `src/app/obd/obd.h`)
with the oil temperature. Still worth checking on a tank of another
blend; and it's this tune's RAM layout, a reflash may move it: rescan
then.

## Finding the stability control's modes on the bus

The GT86 has two buttons for it: TRC OFF and VSC SPORT. Together they
make 5 modes: everything on; TRC off (a short press of TRC OFF); VSC
sport; both; everything off (TRC OFF held a few seconds: VSC off too).
"Listen CAN" on the settings page looks for the bits that tell them
apart, through the dongle (`src/app/obd/sniff.c`).

1. Engine running, parked. Click Listen CAN.
2. The screen prompts each of 8 steps: everything on, TRC OFF, back on,
   VSC SPORT, VSC SPORT and TRC OFF, back on, everything off, back on.
   Do what it says, check the lamps, then click the knob.
3. Hands off while it listens, about 30s a step, until the next prompt:
   about 5 minutes in all.
4. `mise run logs`: a `sniff:` line per candidate ID, with the bits that
   tell the modes apart (`bits`, a mask over its bytes) and its data in
   each mode (`on`, `trc`, `sport`, `sport+trc`, `off`).

The dongle can't print the whole bus over BLE, so it listens to 256 IDs
at a time, 1.5s each, twice over the bus per step. A bit that changes
within a step is noise. A candidate is a bit quiet within every step,
the same in the 4 "on" steps, and different between two modes. It works
bit by bit, because a flag often shares its frame with a rolling
counter: a first run, ID by ID, found 15 of its 36 IDs noisy and no
candidate. The steps back to "on" weed out a value drifting along (a
temperature), except one that ticks exactly once per step: it alternates
like a flag. A real flag is a bit or two, the same across a second run.
Run it twice: the right ID comes back both times.

A first run (2026-11-16, parked, engine running), through TRC OFF and
everything off only (it didn't know VSC SPORT), found 36 IDs, 15 with
noise, and a single candidate, **0x0D3** (7 bytes):

| Mode | Data |
| --- | --- |
| on | `00 06 C0 0F 00 00 E8` |
| TRC OFF | `20 06 C0 0F 00 00 31` |
| everything off | `20 0E C0 0E 00 00 8D` |

Its bits read like the modes: byte 0 bit 5 (0x20) set with TRC off (in
both off modes), byte 1 bit 3 (0x08) set with VSC off, byte 3 bit 0
(0x01) set while VSC is on. The last byte changes every frame (a
checksum or a counter, noise). One run only: a second one must find
the same bits, and VSC SPORT's. The dash already reads TRC off and VSC
off from them (`GT86_ID_DRIVE_MODE`).

## Adding an aftermarket ethanol source

Same idea, if you'd rather get ethanol % over CAN from an aftermarket
controller than from the ECU's RAM: find the controller's documented
frame layout and register a decoder the same way.
