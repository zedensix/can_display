package tests

import (
	"strings"
	"testing"

	"tinygo.org/x/tinyfont/freesans"

	"zedensix/can_display/internal/signals"
	"zedensix/can_display/internal/ui"
)

func TestStateStatus(t *testing.T) {
	s := &signals.State{}
	if got := s.Snapshot().Status; got != "" {
		t.Fatalf("Status = %q, want empty before any SetStatus", got)
	}
	s.SetStatus("OBD DONGLE\nscanning...")
	if got := s.Snapshot().Status; got != "OBD DONGLE\nscanning..." {
		t.Errorf("Status = %q, want the last SetStatus", got)
	}
	s.SetStatus("")
	if got := s.Snapshot().Status; got != "" {
		t.Errorf("Status = %q, want empty after clearing", got)
	}
}

func TestWrapTextKeepsEveryWordAndSplitsLongLines(t *testing.T) {
	text := "BLE connect failed: no known ELM327 serial service on this device, see elmProfiles"
	lines := ui.WrapText(&freesans.Bold9pt7b, text, 216)
	if len(lines) < 2 {
		t.Fatalf("WrapText returned %d line(s), want the long error split: %q", len(lines), lines)
	}
	if got := strings.Join(lines, " "); got != text {
		t.Errorf("rejoined lines = %q, want %q", got, text)
	}
	if lines := ui.WrapText(&freesans.Bold9pt7b, "", 216); len(lines) != 1 || lines[0] != "" {
		t.Errorf("WrapText(\"\") = %q, want one empty line", lines)
	}
}
