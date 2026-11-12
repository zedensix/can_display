package tests

import (
	"testing"

	"zedensix/can_display/internal/sensors"
)

func TestEthanolPercent(t *testing.T) {
	cases := []struct {
		volts float32
		want  float32
	}{
		{0.5, 0},   // sensor min -> 0%
		{4.5, 100}, // sensor max -> 100%
		{2.5, 50},  // midpoint
		{0, 0},     // below sensor min -> clamp to 0, not negative
		{5, 100},   // above sensor max -> clamp to 100
	}
	for _, c := range cases {
		got := sensors.EthanolPercent(c.volts)
		if !approxEqual(got, c.want, 0.01) {
			t.Errorf("sensors.EthanolPercent(%v) = %v, want %v", c.volts, got, c.want)
		}
	}
}
