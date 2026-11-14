// Package ui draws the dashboard (tiles, status screen, boot animation)
// on any TinyGo display, the real panel and the simulator alike.
package ui

import (
	"image/color"
	"math"
	"time"

	"zedensix/can_display/internal/display"
	"zedensix/can_display/internal/settings"
	"zedensix/can_display/internal/signals"
)

// DashTile pairs a Tile's look with the accessor that pulls its current
// reading out of a Snapshot.
type DashTile struct {
	Tile
	Value func(signals.Snapshot) signals.Reading
}

// DefaultTiles is the stock GT86 layout. Coolant, oil temp and RPM have
// real decoders today (internal/canbus/gt86); battery/AFR/ethanol are
// wired up and waiting on a data source -- see that package's doc
// comment. Thresholds/limits live in config.go, not here -- this is
// just wiring labels/formatting onto them.
func DefaultTiles() []DashTile {
	return []DashTile{
		{
			Tile: Tile{
				Label: "RPM", Unit: "", Format: "%.0f", StaleAfter: DefaultStaleAfter,
				Range: RPMRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.RPM },
		},
		{
			Tile: Tile{
				Label: "COOLANT", Unit: "°C", Format: "%.0f", StaleAfter: DefaultStaleAfter,
				Range: CoolantRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.CoolantTempC },
		},
		{
			Tile: Tile{
				Label: "OIL TEMP", Unit: "°C", Format: "%.0f", StaleAfter: DefaultStaleAfter,
				Range: OilTempRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.OilTempC },
		},
		{
			Tile: Tile{
				Label: "BATTERY", Unit: "V", Format: "%.1f", StaleAfter: DefaultStaleAfter,
				Range: BatteryRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.BatteryVoltage },
		},
		{
			Tile: Tile{
				Label: "AFR", Unit: "", Format: "%.1f", StaleAfter: DefaultStaleAfter,
				Range: AFRRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.AFR },
		},
		{
			Tile: Tile{
				Label: "ETHANOL", Unit: "%", Format: "%.0f", StaleAfter: DefaultStaleAfter,
				Range: EthanolRange,
			},
			Value: func(s signals.Snapshot) signals.Reading { return s.EthanolPercent },
		},
	}
}

// Dashboard is the top-level screen: a grid of tiles, one per signal,
// laid out to fill whatever panel it's given.
type Dashboard struct {
	Theme Theme
	Tiles []DashTile
}

// gridGap is the pixel gap left between tiles (and around the screen
// edge) so adjacent tiles don't visually merge into one another.
const gridGap = 3

// Render draws one frame for screen, then flushes it: the settings
// page, the data source's status while it has one, a single gauge, or
// every gauge in a grid.
func (dash Dashboard) Render(d display.Display, snap signals.Snapshot, screen Screen, cfg settings.Settings) error {
	w, h := d.Size()
	fillRect(d, Rect{X: 0, Y: 0, W: w, H: h}, dash.Theme.Background)

	switch {
	case screen.Page == PageSettings:
		drawSettings(d, dash.Theme, screen, cfg)
	case snap.Status != "":
		drawStatus(d, dash.Theme, snap.Status, snap.At)
	case screen.View > 0 && screen.View <= len(dash.Tiles):
		t := dash.Tiles[screen.View-1]
		t.DrawLarge(d, dash.Theme, Rect{X: gridGap, Y: gridGap, W: w - 2*gridGap, H: h - 2*gridGap}, t.Value(snap), snap.At)
	default:
		dash.drawTiles(d, snap.At, func(t DashTile) signals.Reading { return t.Value(snap) })
	}
	return d.Display()
}

const (
	splashDuration = 1500 * time.Millisecond
	splashFade     = 300 * time.Millisecond
	sweepDuration  = 2 * time.Second

	// BootDuration is how long RenderBoot's animation lasts.
	BootDuration = splashDuration + sweepDuration
)

// RenderBoot draws the boot animation, elapsed into it: the logo fades
// in and out, then every gauge sweeps up to its max and back down, like
// a car cluster's self-test at ignition. It reports false, without
// drawing, once BootDuration is over.
func (dash Dashboard) RenderBoot(d display.Display, elapsed time.Duration) (bool, error) {
	if elapsed >= BootDuration {
		return false, nil
	}
	w, h := d.Size()
	fillRect(d, Rect{X: 0, Y: 0, W: w, H: h}, dash.Theme.Background)

	if elapsed < splashDuration {
		fade := min(elapsed, splashDuration-elapsed, splashFade)
		drawLogo(d, blend(dash.Theme.Background, dash.Theme.Danger, float32(fade)/float32(splashFade)))
		return true, d.Display()
	}

	sweepElapsed := elapsed - splashDuration
	sweep := float32(math.Sin(math.Pi * float64(sweepElapsed) / float64(sweepDuration)))
	now := time.Now()
	dash.drawTiles(d, now, func(t DashTile) signals.Reading {
		value := t.Range.Min + sweep*(t.Range.Max-t.Range.Min)
		return signals.Reading{Value: value, Valid: true, UpdatedAt: now}
	})
	return true, d.Display()
}

func (dash Dashboard) drawTiles(d display.Display, now time.Time, reading func(DashTile) signals.Reading) {
	w, h := d.Size()
	const cols = 2
	rows := (int16(len(dash.Tiles)) + cols - 1) / cols

	cellW := w / cols
	cellH := h / rows

	for i, t := range dash.Tiles {
		col := int16(i) % cols
		row := int16(i) / cols
		rect := Rect{
			X: col*cellW + gridGap,
			Y: row*cellH + gridGap,
			W: cellW - 2*gridGap,
			H: cellH - 2*gridGap,
		}
		t.Draw(d, dash.Theme, rect, reading(t), now)
	}
}

// blend mixes from into to: 0 gives from, 1 gives to.
func blend(from, to color.RGBA, amount float32) color.RGBA {
	mix := func(a, b uint8) uint8 { return uint8(float32(a) + (float32(b)-float32(a))*amount) }
	return color.RGBA{R: mix(from.R, to.R), G: mix(from.G, to.G), B: mix(from.B, to.B), A: 255}
}
