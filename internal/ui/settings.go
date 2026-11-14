package ui

import (
	"strconv"

	"tinygo.org/x/tinyfont"

	"zedensix/can_display/internal/display"
	"zedensix/can_display/internal/settings"
)

const (
	settingsMargin    = 8
	settingsTop       = 48
	settingsRowHeight = 40
)

// drawSettings draws the settings page: one row per setting, the one
// under the cursor highlighted, its value in the warning color while
// turning changes it.
func drawSettings(d display.Display, theme Theme, screen Screen, cfg settings.Settings) {
	w, h := d.Size()
	tinyfont.WriteLine(d, labelFont, settingsMargin+4, 28, "SETTINGS", theme.Warning)

	rows := [settingsRows][2]string{
		RowBrightness:  {"Brightness", strconv.Itoa(cfg.Brightness) + "%"},
		RowAutoConnect: {"Auto-connect", onOff(cfg.AutoConnect)},
		RowFlip:        {"Flip screen", onOff(cfg.Flip)},
		RowBack:        {"Back", ""},
	}
	for i, row := range rows {
		y := int16(settingsTop + i*settingsRowHeight)
		labelColor, valueColor := theme.Label, theme.Good
		if i == screen.Cursor {
			fillRect(d, Rect{X: settingsMargin, Y: y, W: w - 2*settingsMargin, H: settingsRowHeight - 6}, theme.Stale)
			labelColor = theme.Warning
			if screen.Editing {
				valueColor = theme.Warning
			}
		}
		tinyfont.WriteLine(d, labelFont, settingsMargin+6, y+23, row[0], labelColor)
		_, width := tinyfont.LineWidth(labelFont, row[1])
		tinyfont.WriteLine(d, labelFont, w-settingsMargin-6-int16(width), y+23, row[1], valueColor)
	}

	tinyfont.WriteLine(d, labelFont, settingsMargin+4, h-14, "hold: reboot", theme.Stale)
}

func onOff(on bool) string {
	if on {
		return "ON"
	}
	return "OFF"
}
