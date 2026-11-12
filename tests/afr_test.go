package tests

import (
	"testing"
	"time"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/gt86"
	"zedensix/can_display/internal/canbus/obd2"
	"zedensix/can_display/internal/signals"
)

func TestAFRPollerSendsExpectedRequest(t *testing.T) {
	bus := &fakeWriter{}
	poller := gt86.AFRPoller(bus)

	if len(poller.Requests) != 1 || poller.Requests[0] != (obd2.Request{Mode: obd2.ModeCurrentData, PID: obd2.PIDEquivalenceRatioO2S1}) {
		t.Fatalf("poller.Requests = %v, want [%v]", poller.Requests, (obd2.Request{Mode: obd2.ModeCurrentData, PID: obd2.PIDEquivalenceRatioO2S1}))
	}

	f := obd2.RequestFrame(poller.Requests[0])
	if err := bus.WriteFrame(f); err != nil {
		t.Fatalf("WriteFrame: %v", err)
	}
	if len(bus.sent) != 1 || bus.sent[0].ID != obd2.FunctionalRequestID {
		t.Fatalf("sent = %v, want one frame to %#x", bus.sent, obd2.FunctionalRequestID)
	}
}

func TestRegisterAFRDecodesResponse(t *testing.T) {
	state := &signals.State{}
	reg := canbus.NewRegistry()
	gt86.Register(reg, state)

	// raw=32768 (0x8000) -> lambda 1.0 -> AFR = 14.7 (stoichiometric).
	response := canbus.Frame{
		ID:   obd2.ResponseIDECU1,
		DLC:  8,
		Data: [8]byte{0x06, 0x41, 0x24, 0x80, 0x00, 0x00, 0x00, 0x00},
	}
	reg.Dispatch(response, time.Now())

	snap := state.Snapshot()
	if !snap.AFR.Valid {
		t.Fatal("AFR should be valid after a well-formed response")
	}
	if diff := snap.AFR.Value - 14.7; diff < -0.01 || diff > 0.01 {
		t.Errorf("AFR = %v, want ~%v", snap.AFR.Value, 14.7)
	}
}

func TestRegisterAFRIgnoresUnrelatedResponses(t *testing.T) {
	state := &signals.State{}
	reg := canbus.NewRegistry()
	gt86.Register(reg, state)

	// Same response ID, but answering a different PID -- must not be
	// mistaken for an AFR reading.
	response := canbus.Frame{
		ID:   obd2.ResponseIDECU1,
		DLC:  8,
		Data: [8]byte{0x04, 0x41, 0x0C, 0x1A, 0xF8, 0x00, 0x00, 0x00}, // PID 0x0C = RPM
	}
	reg.Dispatch(response, time.Now())

	if state.Snapshot().AFR.Valid {
		t.Error("AFR should still be invalid: response was for a different PID")
	}
}

type fakeWriter struct {
	sent []canbus.Frame
}

func (w *fakeWriter) WriteFrame(f canbus.Frame) error {
	w.sent = append(w.sent, f)
	return nil
}
