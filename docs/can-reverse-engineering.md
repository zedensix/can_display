# Verifying and extending the GT86 CAN decoders

## Why this matters

The IDs and formulas in `src/app/obd/obd.c` come from community
reverse-engineering (see the source link in that package's doc comment),
not from Toyota/Subaru factory documentation. Every one the dash shows
reads right on this car (a 2015 GT86, its tune); on another car or a
reflash, check them before trusting what the dash shows.

## What's already wired up

| Signal | Status |
|---|---|
| RPM | Decoded from `0x140` bytes 2-3, 14-bit little-endian, no scaling. Checked on this car. (`0x141` bytes 4-5 carries a second copy, per the same source.) |
| Coolant temp | Decoded from `0x360` byte 3, `value - 40`°C. Checked on this car: the gauge reads right in use. |
| Oil temp | Decoded from `0x360` byte 2, `value - 40`°C. Checked on this car: the gauge reads right in use. |
| AFR | Polled via OBD-II Mode 01 PID 0x24 (standard, not reverse-engineered) from the factory wideband sensor. Checked on this car. |
| Battery voltage | Polled via OBD-II Mode 01 PID 0x42, the engine ECU's supply, a few tenths under the battery's. Checked on this car. No broadcast found. |
| Ethanol % | Not on CAN nor a PID: read from the engine ECU's RAM (below). Checked on this car against OFT. |

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
   keep up with a request every 250ms; slow it down if not.
4. If a functional-broadcast request (`0x7DF`, what this project sends
   by default) gets no response, try a *physical* request to `0x7E0`
   instead (the engine ECU's usual address) -- some ECUs are picky
   about which addressing mode they answer. That'd mean changing
   `OBD_REQUEST_ID` (`src/app/obd/obd.h`) to `0x7E0`.
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

"Scan ECU RAM", on the settings page while the dash read the car
through an ELM327 dongle, scanned it (2 to 3 minutes, polling paused)
and logged every float on that grid, as an ethanol % or as the sensor's
voltage. It went with the dongle: `git show 9dc0432:src/app/obd/elm_scan.c`. To pin the address down, compare the
candidates with what OFT or another logger shows at the same time, then
across two tanks with different blends: the right address follows the
blend.

Three scans on one tank (2026-11-15 and 16, engine running) found a
single address every time: **0xFFF87C7C**, at 66.66, 65.72 and 66.79 %
(the third scan also had 66.785 at 0xFFF8D410, a copy), and it matched
OFT's reading. The dash polls it (`ETHANOL_RAM_READ` in `src/app/obd/obd.h`)
in the MCP2515's slower round of requests. Still worth checking on a
tank of another blend; and it's this tune's RAM layout, a reflash may
move it: rescan then.

## Finding the stability control's modes on the bus

The GT86 has two buttons for it: TRC OFF and VSC SPORT. Together they
make 5 modes: everything on; TRC off (a short press of TRC OFF); VSC
sport; both; everything off (TRC OFF held a few seconds: VSC off too).
"Listen CAN", on the settings page while the dash read the car through
an ELM327 dongle, looked for the bits that tell them apart and found
them (below). It went with the dongle (its last version: `git show
9dc0432:src/app/obd/sniff.c`); this run's steps are in `git show
48b9d51:src/app/obd/sniff.c`. How it ran:

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
| on | `00 06 C0 0F 00 00 2B` |
| TRC OFF | `20 06 C0 0F 00 00 26` |
| VSC SPORT | `08 0E C0 0F 00 00 A0` |
| VSC SPORT + TRC OFF | `28 0E C0 0F 00 00 AA` |
| everything off | `20 0E C0 0E 00 00 A6` |

A first run (on, TRC OFF, everything off) found 0x0D3 alone; the second,
through all 5 modes, found the same bits and VSC SPORT's:

- byte 0 bit 5 (0x20): TRC off (TRC OFF, VSC SPORT + TRC OFF, everything off);
- byte 0 bit 3 (0x08): VSC sport, alone or with TRC OFF;
- byte 3 bit 0 (0x01): cleared with everything off;
- byte 1 bit 3 (0x08): set in VSC SPORT and everything off alike, so it
  tells neither (the first run, without VSC SPORT, mistook it for VSC
  off).

The last byte changes every frame (a checksum or a counter, noise). The
second run also flagged 0x141 byte 2 bit 4, set in the TRC OFF step
only: that byte drifts down step after step (85, 7C, 6B, 6A, 68), an
analog value whose bit happened to line up, not a flag. The dash reads
all 5 modes from 0x0D3 (`GT86_ID_DRIVE_MODE`); still to check against
the cluster's own lamps on a drive.

## Finding the doors, the boot and the cruise control stalk

"Listen CAN" then looked for the doors, the boot, the cruise control
stalk and its main switch: the door screen and a stalk driving the dash
need their bits. Contact on, engine off, parked:

1. Settings, Listen CAN.
2. The screen prompts each of 15 steps: everything shut and the cruise
   control off, the driver's door open, shut, the passenger's open,
   shut, the boot open, shut, the stalk held up, let go, held down, let
   go, held pulled, let go, the cruise ON/OFF button pressed (CRUISE
   lit), pressed again. Do what it says, then Next; Cancel ends it at
   any step.
3. Keep it so while it listens, about 25s a step, until the next
   prompt: a stalk step means holding it all along. About 7 minutes in
   all.
4. `mise run logs`: after a legend, a `sniff:` line per candidate ID,
   its bits (`bits`, a mask over its bytes) and its data in each state,
   a letter each: S shut, D driver's door, P passenger's, B boot, U
   stalk up, W down, C pulled, M cruise on.

Not found isn't proof it's not there: the stalk may be wired to the
engine ECU alone, its presses never on the bus; then the ECU's RAM is
the next place to look.

The first run on this car (2026-11-17, contact on, engine off) found:

- the doors and the boot in 0x375 (`GT86_ID_DOORS`), byte 1: bit 0 the
  driver's door, bit 1 the passenger's, bit 5 the boot; byte 3 bit 2 is
  set with either door open. 0x374 byte 3 has the same three bits. The
  dash reads 0x375 as the MCP2515 streams it;
- the stalk in 0x144 byte 0: bit 4 held up, bit 5 pulled. Held down and
  the ON/OFF button changed nothing on the bus: perhaps they only count
  with the engine running, a run to redo engine on (Listen CAN would
  need porting to the MCP2515 first). The dash doesn't read the stalk.

## Adding an aftermarket ethanol source

Same idea, if you'd rather get ethanol % over CAN from an aftermarket
controller than from the ECU's RAM: find the controller's documented
frame layout and register a decoder the same way.
