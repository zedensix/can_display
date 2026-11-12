// Package obd2 implements just enough of ISO 15765-4 (CAN-based OBD-II)
// request/response to poll standard Mode 01 PIDs and decode their
// replies. It does NOT implement multi-frame ISO-TP reassembly (flow
// control, consecutive frames) -- every PID this project currently
// polls fits entirely in one CAN frame's response, so that's out of
// scope until something needs it.
//
// Unlike internal/canbus/gt86's passive decoders, OBD-II PIDs aren't
// broadcast on their own -- the ECU only replies to an explicit
// request. See Poller.
package obd2

import (
	"time"

	"zedensix/can_display/internal/canbus"
)

const (
	// FunctionalRequestID is the standard OBD-II broadcast request
	// address: "any ECU that implements this PID, please answer."
	// Some vehicles respond better to a physical request instead (a
	// specific ECU's address, typically 0x7E0 for the engine ECU) --
	// worth trying if the functional address gets no reply.
	FunctionalRequestID uint32 = 0x7DF

	// ResponseIDECU1 is the standard reply address for the first (on
	// most single-ECU-relevant setups, the only one that matters)
	// responding ECU.
	ResponseIDECU1 uint32 = 0x7E8

	// ModeCurrentData is OBD-II "Mode 01": request current live data.
	ModeCurrentData byte = 0x01
)

// Request is one Mode 01 PID to poll for.
type Request struct {
	Mode, PID byte
}

// RequestFrame builds a single-frame ISO 15765-4 request for req, e.g.
// RequestFrame(Request{ModeCurrentData, 0x24}) asks for "Mode 01 PID
// 0x24". The frame is sent to FunctionalRequestID and padded to 8
// bytes with zeros, per the ISO 15765-4 standard framing.
func RequestFrame(req Request) canbus.Frame {
	return canbus.Frame{
		ID:  FunctionalRequestID,
		DLC: 8,
		// byte 0: ISO-TP PCI, single frame with 2 bytes of payload
		// (mode + PID) following.
		Data: [8]byte{0x02, req.Mode, req.PID, 0, 0, 0, 0, 0},
	}
}

// ParseSingleFrameResponse extracts the payload bytes from a
// single-frame ISO 15765-4 response to a Mode 01 request, e.g. as sent
// to ResponseIDECU1. It returns ok=false for anything that isn't a
// well-formed single-frame positive response to exactly this
// mode/PID -- multi-frame responses, negative responses ("no data"),
// and mismatched PIDs are all rejected rather than guessed at.
func ParseSingleFrameResponse(f canbus.Frame, req Request) (payload []byte, ok bool) {
	if f.DLC < 3 {
		return nil, false
	}
	pci := f.Data[0]
	if pci&0xF0 != 0 {
		// Not a single frame (0x1_ = first frame of multi-frame, 0x2_ =
		// consecutive frame, 0x3_ = flow control) -- unsupported here.
		return nil, false
	}
	length := int(pci & 0x0F)
	if length < 2 || 1+length > 8 {
		return nil, false
	}
	// A positive response echoes the request mode with the high bit of
	// that byte's nibble set (Mode 0x01 -> 0x41), then the PID.
	if f.Data[1] != req.Mode+0x40 || f.Data[2] != req.PID {
		return nil, false
	}
	return f.Data[3 : 1+length], true
}

// Poller periodically sends each configured Request over bus, cycling
// through them in order. Responses aren't read here -- they arrive on
// the bus's normal receive path (ResponseIDECU1) like any other frame,
// and get decoded by whatever's registered against that ID in a
// canbus.Registry. Run this in its own goroutine.
type Poller struct {
	Bus      canbus.Writer
	Requests []Request
	Interval time.Duration
}

// Run sends each request in turn, one per Interval, forever, until a
// write fails.
func (p Poller) Run() error {
	if len(p.Requests) == 0 {
		return nil
	}
	i := 0
	for {
		if err := p.Bus.WriteFrame(RequestFrame(p.Requests[i])); err != nil {
			return err
		}
		i = (i + 1) % len(p.Requests)
		time.Sleep(p.Interval)
	}
}
