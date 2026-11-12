package tests

import (
	"testing"

	"zedensix/can_display/internal/ui"
)

func TestRangeColorFor(t *testing.T) {
	r := ui.Range{Min: 0, Max: 100, WarnLow: 20, WarnHigh: 80, DangerLow: 10, DangerHigh: 90}
	theme := ui.Dark

	cases := []struct {
		value float32
		want  string
	}{
		{5, "danger"},   // below DangerLow
		{15, "warning"}, // between DangerLow and WarnLow
		{50, "good"},    // inside the safe band
		{85, "warning"}, // between WarnHigh and DangerHigh
		{95, "danger"},  // above DangerHigh
	}

	for _, c := range cases {
		got := r.ColorFor(c.value, theme)
		var want = theme.Good
		switch c.want {
		case "danger":
			want = theme.Danger
		case "warning":
			want = theme.Warning
		}
		if got != want {
			t.Errorf("ColorFor(%v) = %v, want %s (%v)", c.value, got, c.want, want)
		}
	}
}

func TestRangeFractionClamps(t *testing.T) {
	r := ui.Range{Min: 0, Max: 100}

	if f := r.Fraction(-50); f != 0 {
		t.Errorf("Fraction(-50) = %v, want 0", f)
	}
	if f := r.Fraction(150); f != 1 {
		t.Errorf("Fraction(150) = %v, want 1", f)
	}
	if f := r.Fraction(25); f != 0.25 {
		t.Errorf("Fraction(25) = %v, want 0.25", f)
	}
}
