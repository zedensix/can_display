package tests

import (
	"testing"

	"zedensix/can_display/internal/settings"
	"zedensix/can_display/internal/ui"
)

func TestScreenTurnCyclesGaugeViews(t *testing.T) {
	var s ui.Screen
	cfg := settings.Default
	s.Turn(2, 6, &cfg)
	if s.View != 2 {
		t.Errorf("View = %d after 2 detents, want 2", s.View)
	}
	s.Turn(-3, 6, &cfg)
	if s.View != 6 {
		t.Errorf("View = %d after going back past the grid, want the last gauge (6)", s.View)
	}
	s.Turn(1, 6, &cfg)
	if s.View != 0 {
		t.Errorf("View = %d after the last gauge, want back to the grid", s.View)
	}
}

func TestScreenSettingsPage(t *testing.T) {
	var s ui.Screen
	cfg := settings.Default
	s.Press(&cfg)
	if s.Page != ui.PageSettings || s.Cursor != ui.RowBrightness {
		t.Fatalf("after a click on the gauges: page %d, cursor %d, want settings on brightness", s.Page, s.Cursor)
	}

	s.Turn(-1, 6, &cfg)
	if cfg.Brightness != settings.MaxBrightness {
		t.Errorf("Brightness = %d, turning without editing must move the cursor, not the value", cfg.Brightness)
	}
	s.Press(&cfg)
	s.Turn(-3, 6, &cfg)
	s.Press(&cfg)
	if cfg.Brightness != settings.MaxBrightness-3*settings.BrightnessStep || s.Editing {
		t.Errorf("Brightness = %d, editing %v, want 3 steps down and editing done", cfg.Brightness, s.Editing)
	}
	s.Press(&cfg)
	s.Turn(-50, 6, &cfg)
	s.Press(&cfg)
	if cfg.Brightness != settings.MinBrightness {
		t.Errorf("Brightness = %d, want it stopped at the minimum", cfg.Brightness)
	}

	s.Turn(1, 6, &cfg)
	s.Press(&cfg)
	if !cfg.AutoConnect {
		t.Error("a click on auto-connect didn't turn it on")
	}
	s.Turn(1, 6, &cfg)
	s.Press(&cfg)
	if !cfg.Flip {
		t.Error("a click on flip didn't turn it on")
	}
	s.Turn(10, 6, &cfg)
	s.Press(&cfg)
	if s.Page != ui.PageGauges {
		t.Errorf("Page = %d after a click on Back, want the gauges", s.Page)
	}
}

func TestSettingsRoundTrip(t *testing.T) {
	cfg := settings.Settings{Brightness: 40, AutoConnect: true, Flip: true}
	if got := settings.Decode(cfg.Encode()); got != cfg {
		t.Errorf("Decode(Encode(%+v)) = %+v", cfg, got)
	}
	erased := make([]byte, settings.Size)
	for i := range erased {
		erased[i] = 0xFF
	}
	if got := settings.Decode(erased); got != settings.Default {
		t.Errorf("Decode(erased flash) = %+v, want the defaults", got)
	}
}
