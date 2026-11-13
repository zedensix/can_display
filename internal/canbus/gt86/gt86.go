// Package gt86 decodes the factory CAN bus of the "Gen1" Toyota
// 86 / Subaru BRZ / Scion FR-S (2012-2020 model years, which covers a
// 2015 GT86).
//
// IMPORTANT: these IDs and formulas come from community
// reverse-engineering, not from any Toyota/Subaru factory
// documentation, and have not been verified against this specific car.
// Treat every value here as "probably right, please confirm" -- see
// /docs/can-reverse-engineering.md for how to check them with a cheap
// USB-CAN adapter before trusting the dash.
//
// Primary source: https://github.com/timurrrr/ft86/blob/main/can_bus/gen1.md
// (captured from the OBD-II port, pins 6/14 (CAN H/L), on a Gen1
// 86/BRZ/FR-S.)
package gt86

import (
	"time"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/signals"
)

// IDTemperatures (0x360, ~20Hz) carries oil and coolant temperature
// among other things (cruise control state). Both temperatures are
// encoded as (raw byte - 40) degrees Celsius.
const IDTemperatures uint32 = 0x360

// IDThrottleRPM (0x140, ~100Hz) carries accelerator position and
// engine RPM. RPM is a 14-bit little-endian value starting at bit 16
// (i.e. byte-aligned at bytes 2-3), with no scaling applied -- the raw
// value is the RPM directly. (0x141 carries a second copy of RPM at
// bit 32/bytes 4-5, if this one ever needs cross-checking.)
const IDThrottleRPM uint32 = 0x140

// AFR is decoded from OBD-II Mode 01 PID 0x24, not a passively
// broadcast ID -- see afr.go for why that requires more than just
// Register (a Poller has to actively request it too), and for the
// caveats on whether it'll actually work on this car.
//
// Not currently decoded at all:
//
//   - Battery voltage: not found on the factory bus in any Gen1/Gen2
//     community reverse-engineering doc so far. Needs further
//     reverse-engineering work. If you find it, add a decoder here.
//     (The ELM327 source reads it from the dongle's own ATRV instead.)
//
// Ethanol percentage doesn't go through this package at all -- it's
// read from a locally-wired analog sensor, not CAN. See
// internal/sensors and cmd/firmware's updateEthanolPercent.

// Register wires up every known passive decoder into reg, writing
// results into state. AFR additionally needs an obd2.Poller actively
// running against the same bus -- see AFRPoller.
func Register(reg *canbus.Registry, state *signals.State) {
	reg.Register(IDTemperatures, decodeTemperatures(state))
	reg.Register(IDThrottleRPM, decodeRPM(state))
	registerAFR(reg, state)
}

func decodeTemperatures(state *signals.State) canbus.Decoder {
	return func(f canbus.Frame, at time.Time) {
		if f.DLC < 4 {
			return
		}
		oilC := float32(f.Data[2]) - 40
		coolantC := float32(f.Data[3]) - 40
		state.SetOilTempC(oilC, at)
		state.SetCoolantTempC(coolantC, at)
	}
}

func decodeRPM(state *signals.State) canbus.Decoder {
	return func(f canbus.Frame, at time.Time) {
		if f.DLC < 4 {
			return
		}
		raw := uint16(f.Data[2]) | uint16(f.Data[3])<<8
		rpm := raw & 0x3FFF // low 14 bits
		state.SetRPM(float32(rpm), at)
	}
}
