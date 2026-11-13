package ui

import (
	"fmt"
	"image/color"
	"time"

	"tinygo.org/x/tinyfont"
	"tinygo.org/x/tinyfont/freesans"

	"zedensix/can_display/internal/display"
	"zedensix/can_display/internal/signals"
)

// labelFont/valueFonts are shared across every tile so the whole
// dashboard has one consistent type scale. They're deliberately not
// configurable per-tile -- if you want a different look, change it here
// and every tile follows.
//
// valueFonts is tried largest-first in fitValueFont: most readings
// ("8000", "16.5") fit fine at full size, but wider ones -- an extra
// digit, a unit suffix, a leading "-" ("-14.2V") -- can be too wide for
// a tile at 24pt and need to drop a size (or two) to stay inside it
// instead of overflowing into the next tile or off the panel edge.
var (
	labelFont  = &freesans.Bold9pt7b
	valueFonts = []*tinyfont.Font{
		&freesans.Bold24pt7b,
		&freesans.Bold18pt7b,
		&freesans.Bold12pt7b,
	}
)

// fitValueFont returns the largest font in valueFonts that renders s
// within maxWidth pixels, falling back to the smallest if even that
// overflows.
func fitValueFont(s string, maxWidth int16) *tinyfont.Font {
	for _, f := range valueFonts {
		_, outer := tinyfont.LineWidth(f, s)
		if int16(outer) <= maxWidth {
			return f
		}
	}
	return valueFonts[len(valueFonts)-1]
}

// Rect is a pixel rectangle on the display.
type Rect struct {
	X, Y, W, H int16
}

// Range defines the thresholds a Tile uses to color its value and to
// place it on the fill bar.
type Range struct {
	Min, Max              float32 // bar endpoints
	WarnLow, WarnHigh     float32 // outside this band -> Warning color
	DangerLow, DangerHigh float32 // outside this band -> Danger color
}

func (r Range) ColorFor(v float32, t Theme) color.RGBA {
	switch {
	case v <= r.DangerLow || v >= r.DangerHigh:
		return t.Danger
	case v <= r.WarnLow || v >= r.WarnHigh:
		return t.Warning
	default:
		return t.Good
	}
}

func (r Range) Fraction(v float32) float32 {
	if r.Max <= r.Min {
		return 0
	}
	f := (v - r.Min) / (r.Max - r.Min)
	switch {
	case f < 0:
		return 0
	case f > 1:
		return 1
	default:
		return f
	}
}

// Tile is a single rectangular gauge: a label, a big numeric value with
// a unit suffix, and a horizontal fill bar colored by Range.
type Tile struct {
	Label  string
	Unit   string
	Format string // fmt verb for the value, e.g. "%.0f" or "%.1f"
	Range  Range
	// StaleAfter is how long since the last update before this tile
	// shows "--" instead of a frozen last value.
	StaleAfter time.Duration
}

// Draw renders the tile inside rect using reading as its current value.
func (t Tile) Draw(d display.Display, theme Theme, rect Rect, reading signals.Reading, now time.Time) {
	fillRect(d, rect, theme.Background)

	labelX, labelY := rect.X+8, rect.Y+18
	tinyfont.WriteLine(d, labelFont, labelX, labelY, t.Label, theme.Label)

	stale := reading.Stale(now, t.StaleAfter)

	valueStr := "--"
	valColor := theme.Stale
	frac := float32(0)
	if !stale {
		valueStr = fmt.Sprintf(t.Format, reading.Value) + t.Unit
		valColor = t.Range.ColorFor(reading.Value, theme)
		frac = t.Range.Fraction(reading.Value)
	}

	// Margin matches labelX's left offset so the value stays visually
	// aligned with the label above it and leaves an equal gap on the
	// right instead of touching (or crossing) the tile's edge.
	const valueMargin = 8
	maxValueWidth := rect.W - 2*valueMargin

	valueY := labelY + 40
	tinyfont.WriteLine(d, fitValueFont(valueStr, maxValueWidth), rect.X+valueMargin, valueY, valueStr, valColor)

	barRect := Rect{X: rect.X + 8, Y: rect.Y + rect.H - 12, W: rect.W - 16, H: 6}
	drawBar(d, barRect, frac, valColor, theme.Stale)
}

// rectFiller is implemented by displays that fill a rectangle faster
// than pixel by pixel: the firmware's framebuffer, the ST7789 driver.
type rectFiller interface {
	FillRectangle(x, y, width, height int16, c color.RGBA) error
}

func fillRect(d display.Display, r Rect, c color.RGBA) {
	if filler, ok := d.(rectFiller); ok && filler.FillRectangle(r.X, r.Y, r.W, r.H, c) == nil {
		return
	}
	for y := r.Y; y < r.Y+r.H; y++ {
		for x := r.X; x < r.X+r.W; x++ {
			d.SetPixel(x, y, c)
		}
	}
}

func drawBar(d display.Display, r Rect, frac float32, fillColor, trackColor color.RGBA) {
	fillRect(d, r, trackColor)
	filled := int16(float32(r.W) * frac)
	if filled > 0 {
		fillRect(d, Rect{X: r.X, Y: r.Y, W: filled, H: r.H}, fillColor)
	}
}
