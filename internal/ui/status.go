package ui

import (
	"strings"
	"time"

	"tinygo.org/x/tinyfont"

	"zedensix/can_display/internal/display"
)

const (
	statusMargin     = 12
	statusLineHeight = 24
)

// drawStatus fills the screen with the data source's status instead of
// the tiles, which would only show "--" until data flows, and a spinner
// so a slow step doesn't look frozen.
func drawStatus(d display.Display, theme Theme, status string, now time.Time) {
	w, h := d.Size()
	drawSpinner(d, theme, w/2, h-48, now)

	y := int16(statusMargin + statusLineHeight)
	for i, paragraph := range strings.Split(status, "\n") {
		color := theme.Label
		if i == 0 {
			color = theme.Warning
		}
		for _, line := range WrapText(paragraph, w-2*statusMargin) {
			tinyfont.WriteLine(d, labelFont, statusMargin, y, line, color)
			y += statusLineHeight
		}
	}
}

// WrapText splits s into lines that fit maxWidth pixels in the status
// font, breaking between words. A single word wider than maxWidth gets
// a line of its own and is clipped by the panel edge.
func WrapText(s string, maxWidth int16) []string {
	var lines []string
	line := ""
	for _, word := range strings.Fields(s) {
		candidate := word
		if line != "" {
			candidate = line + " " + word
		}
		if _, width := tinyfont.LineWidth(labelFont, candidate); line != "" && int16(width) > maxWidth {
			lines = append(lines, line)
			candidate = word
		}
		line = candidate
	}
	if line != "" || len(lines) == 0 {
		lines = append(lines, line)
	}
	return lines
}

// spinnerDots are the offsets of the spinner's dots around its center,
// clockwise from the top.
var spinnerDots = [8][2]int16{{0, -16}, {11, -11}, {16, 0}, {11, 11}, {0, 16}, {-11, 11}, {-16, 0}, {-11, -11}}

const spinnerDotSize = 6

// drawSpinner draws a ring of dots whose bright head moves one dot every
// 100ms, the render loop's pace, trailed by a dimmer dot.
func drawSpinner(d display.Display, theme Theme, centerX, centerY int16, now time.Time) {
	head := int(now.UnixMilli()/100) % len(spinnerDots)
	for i, offset := range spinnerDots {
		color := theme.Stale
		switch (head - i + len(spinnerDots)) % len(spinnerDots) {
		case 0:
			color = theme.Warning
		case 1:
			color = theme.Label
		}
		x := centerX + offset[0] - spinnerDotSize/2
		y := centerY + offset[1] - spinnerDotSize/2
		fillRect(d, Rect{X: x, Y: y, W: spinnerDotSize, H: spinnerDotSize}, color)
	}
}
