package ui

import (
	"math"
	"time"

	"zedensix/can_display/internal/display"
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

// Render lays the tiles out in a grid sized to d and draws every one
// against snap, then flushes the frame. While snap.Status is set, it
// draws the status screen instead.
func (dash Dashboard) Render(d display.Display, snap signals.Snapshot) error {
	w, h := d.Size()
	fillRect(d, Rect{X: 0, Y: 0, W: w, H: h}, dash.Theme.Background)

	if snap.Status != "" {
		drawStatus(d, dash.Theme, snap.Status, snap.At)
		return d.Display()
	}

	dash.drawTiles(d, snap.At, func(t DashTile) signals.Reading { return t.Value(snap) })
	return d.Display()
}

// BootDuration is how long RenderBoot's animation lasts.
const BootDuration = 2 * time.Second

// RenderBoot draws the boot animation, elapsed into it: every gauge
// sweeps up to its max and back down, like a car cluster's self-test at
// ignition. It reports false, without drawing, once BootDuration is over.
func (dash Dashboard) RenderBoot(d display.Display, elapsed time.Duration) (bool, error) {
	if elapsed >= BootDuration {
		return false, nil
	}
	sweep := float32(math.Sin(math.Pi * float64(elapsed) / float64(BootDuration)))
	now := time.Now()

	w, h := d.Size()
	fillRect(d, Rect{X: 0, Y: 0, W: w, H: h}, dash.Theme.Background)
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
