package ui

import "zedensix/can_display/internal/settings"

// Page is a full screen the rotary encoder's click switches between.
type Page int

const (
	PageGauges Page = iota
	PageSettings
)

// Rows of the settings page, top to bottom.
const (
	RowBrightness = iota
	RowAutoConnect
	RowFlip
	RowBack
	settingsRows
)

// Screen is what the rotary encoder drives (the simulator's keyboard
// stands in for it).
//
// On the gauges, turning cycles the views: every gauge, then each one
// alone. A click opens the settings. There, turning moves the cursor, a
// click toggles the row under it or, on brightness, makes turning change
// the value until the next click. Back returns to the gauges.
type Screen struct {
	Page Page
	// View is 0 for every gauge, k for the k-th gauge alone.
	View int
	// Cursor is the selected settings row.
	Cursor int
	// Editing makes turning change the brightness.
	Editing bool
}

// Turn applies encoder detents, positive clockwise. views is how many
// gauges the single-gauge views cycle through.
func (s *Screen) Turn(detents, views int, cfg *settings.Settings) {
	if detents == 0 {
		return
	}
	switch {
	case s.Page == PageGauges:
		count := views + 1
		s.View = ((s.View+detents)%count + count) % count
	case s.Editing:
		cfg.Brightness = min(max(cfg.Brightness+detents*settings.BrightnessStep, settings.MinBrightness), settings.MaxBrightness)
	default:
		s.Cursor = min(max(s.Cursor+detents, 0), settingsRows-1)
	}
}

// Press applies a click.
func (s *Screen) Press(cfg *settings.Settings) {
	if s.Page == PageGauges {
		s.Page, s.Cursor, s.Editing = PageSettings, RowBrightness, false
		return
	}
	switch s.Cursor {
	case RowBrightness:
		s.Editing = !s.Editing
	case RowAutoConnect:
		cfg.AutoConnect = !cfg.AutoConnect
	case RowFlip:
		cfg.Flip = !cfg.Flip
	case RowBack:
		s.Page = PageGauges
	}
}
