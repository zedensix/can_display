package gt86

import (
	"time"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/obd2"
	"zedensix/can_display/internal/signals"
)

// AFR comes from the factory wide-range A/F sensor via standard OBD-II
// Mode 01 PID 0x24 (O2 Sensor 1, equivalence ratio) -- NOT from a
// passively-broadcast CAN ID like coolant/oil temp is. The ECU only
// sends this in response to an explicit request, which is why the
// firmware also needs to run an obd2.Poller (see AFRPoller) in
// parallel with the passive canbus.Registry the rest of this package's
// decoders rely on.
//
// This is more speculative than this package's other decoders, in a
// different way than "the ID might be wrong":
//   - PID 0x24 itself is a standard, SAE J1979-documented PID, not
//     reverse-engineered guesswork -- the wire format is solid.
//   - Whether *this* ECU actually replies usefully to it over the
//     OBD-II port hasn't been confirmed here -- Toyota's exact
//     ISO-15765 addressing/timing on this platform is unverified. If
//     FunctionalRequestID gets no response, try a physical request to
//     0x7E0 instead (see obd2.FunctionalRequestID's doc comment).
//   - Even if it works: multiple independent sources (EcuTek's own
//     docs among them) report the factory sensor's ECU-side scaling
//     can't show richer than ~12.2:1 (0.83 lambda). Fine for
//     cruise/part-throttle monitoring, not for reading a rich WOT pull
//     accurately.
const afrPollInterval = 250 * time.Millisecond

// gasolineStoichAFR is the stoichiometric air-fuel ratio for pure
// gasoline (lambda 1.0). It shifts with ethanol content (E85 is closer
// to 9.8:1); this doesn't correct for that yet, so AFR will read a bit
// rich-of-actual on high-ethanol blends. Worth revisiting once
// EthanolPercent (internal/sensors) is reliably online, since both
// numbers would then be available at once.
const gasolineStoichAFR = 14.7

var afrRequest = obd2.Request{Mode: obd2.ModeCurrentData, PID: obd2.PIDEquivalenceRatioO2S1}

// AFRPoller returns a Poller that requests PID 0x24 on bus, at a rate
// the ECU can be expected to keep up with. Run it in its own goroutine
// alongside canbus.Registry.Run -- the response it triggers comes back
// through the registry like any other frame, decoded by the handler
// registerAFR installs.
func AFRPoller(bus canbus.Writer) obd2.Poller {
	return obd2.Poller{
		Bus:      bus,
		Requests: []obd2.Request{afrRequest},
		Interval: afrPollInterval,
	}
}

func registerAFR(reg *canbus.Registry, state *signals.State) {
	reg.Register(obd2.ResponseIDECU1, func(f canbus.Frame, at time.Time) {
		payload, ok := obd2.ParseSingleFrameResponse(f, afrRequest)
		if !ok {
			return
		}
		lambda, ok := obd2.EquivalenceRatio(payload)
		if !ok {
			return
		}
		state.SetAFR(lambda*gasolineStoichAFR, at)
	})
}
