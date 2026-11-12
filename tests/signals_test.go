package tests

import (
	"testing"
	"time"

	"zedensix/can_display/internal/signals"
)

func TestReadingStale(t *testing.T) {
	now := time.Now()

	unset := signals.Reading{}
	if !unset.Stale(now, time.Second) {
		t.Error("a never-set signals.Reading should be stale")
	}

	fresh := signals.Reading{Value: 42, Valid: true, UpdatedAt: now}
	if fresh.Stale(now, time.Second) {
		t.Error("a signals.Reading updated right now should not be stale")
	}

	old := signals.Reading{Value: 42, Valid: true, UpdatedAt: now.Add(-10 * time.Second)}
	if !old.Stale(now, time.Second) {
		t.Error("a signals.Reading older than maxAge should be stale")
	}
}

func TestStateSetAndSnapshot(t *testing.T) {
	s := &signals.State{}
	now := time.Now()

	s.SetCoolantTempC(88, now)
	s.SetBatteryVoltage(13.8, now)

	snap := s.Snapshot()
	if !snap.CoolantTempC.Valid || snap.CoolantTempC.Value != 88 {
		t.Errorf("CoolantTempC = %+v, want valid 88", snap.CoolantTempC)
	}
	if !snap.BatteryVoltage.Valid || snap.BatteryVoltage.Value != 13.8 {
		t.Errorf("BatteryVoltage = %+v, want valid 13.8", snap.BatteryVoltage)
	}
	// Never set -- should stay invalid rather than reading as zero.
	if snap.AFR.Valid {
		t.Error("AFR should still be invalid: never set")
	}
}
