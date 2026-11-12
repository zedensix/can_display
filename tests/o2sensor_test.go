package tests

import (
	"testing"

	"zedensix/can_display/internal/canbus/obd2"
)

func TestEquivalenceRatio(t *testing.T) {
	cases := []struct {
		payload    []byte
		wantOK     bool
		wantApprox float32
	}{
		{[]byte{0x00, 0x00, 0, 0}, true, 0},   // raw=0 -> lambda 0
		{[]byte{0xFF, 0xFF, 0, 0}, true, 2.0}, // raw=65535 -> lambda ~2.0
		{[]byte{0x80, 0x00, 0, 0}, true, 1.0}, // raw=32768 -> lambda 1.0 (stoichiometric)
		{[]byte{0x01}, false, 0},              // too short
		{nil, false, 0},
	}
	for _, c := range cases {
		got, ok := obd2.EquivalenceRatio(c.payload)
		if ok != c.wantOK {
			t.Errorf("obd2.EquivalenceRatio(%v) ok = %v, want %v", c.payload, ok, c.wantOK)
			continue
		}
		if !ok {
			continue
		}
		diff := got - c.wantApprox
		if diff < 0 {
			diff = -diff
		}
		if diff > 0.001 {
			t.Errorf("obd2.EquivalenceRatio(%v) = %v, want ~%v", c.payload, got, c.wantApprox)
		}
	}
}
