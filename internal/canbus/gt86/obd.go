package gt86

import (
	"time"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/obd2"
	"zedensix/can_display/internal/signals"
)

// RegisterOBD wires up decoders for standard Mode 01 PIDs carrying the
// same values the passive decoders in gt86.go read off the broadcast
// frames (RPM, coolant temp). Use it when the data source can only do
// request/response, like an ELM327 dongle (see internal/elm327), which
// never sees 0x140/0x360. Call Register too: it installs the AFR
// decoder, and its passive decoders simply never fire on such a source.
//
// Oil temperature has no standard PID (Toyota exposes it through a
// manufacturer-specific mode), so it stays "--" on an OBD-only source.
func RegisterOBD(reg *canbus.Registry, state *signals.State) {
	reg.Register(obd2.ResponseIDECU1, pidDecoder(rpmRequest, obd2.EngineRPM, state.SetRPM))
	reg.Register(obd2.ResponseIDECU1, pidDecoder(coolantRequest, obd2.CoolantTempC, state.SetCoolantTempC))
}

var (
	rpmRequest     = obd2.Request{Mode: obd2.ModeCurrentData, PID: obd2.PIDEngineRPM}
	coolantRequest = obd2.Request{Mode: obd2.ModeCurrentData, PID: obd2.PIDCoolantTemp}
)

// OBDPoller returns a Poller cycling through every PID the dashboard
// needs on an OBD-only source. RPM is listed more often than the rest
// because it's the only one that changes faster than a human can read.
// Interval is zero on purpose: it's meant for a Writer whose WriteFrame
// blocks until the answer is back (elm327.Bus), so the dongle's own
// round-trip time paces the loop. Don't hand it a fire-and-forget Writer
// like the MCP2515 one, it would flood the bus.
func OBDPoller(bus canbus.Writer) obd2.Poller {
	return obd2.Poller{
		Bus:      bus,
		Requests: []obd2.Request{rpmRequest, afrRequest, rpmRequest, coolantRequest},
		Interval: 0,
	}
}

func pidDecoder(req obd2.Request, decode func([]byte) (float32, bool), set func(float32, time.Time)) canbus.Decoder {
	return func(f canbus.Frame, at time.Time) {
		payload, ok := obd2.ParseSingleFrameResponse(f, req)
		if !ok {
			return
		}
		if v, ok := decode(payload); ok {
			set(v, at)
		}
	}
}
