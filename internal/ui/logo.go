package ui

import (
	_ "embed"
	"image/color"

	"zedensix/can_display/internal/display"
)

// logoBitmap is the boot splash, generated from 86.png by the logo mise
// task (see cmd/logo for the format). A string, not a []byte, so TinyGo
// leaves it in flash instead of copying it to RAM.
//
//go:embed logo.bin
var logoBitmap string

// drawLogo draws logoBitmap centered on d, its inked pixels in c.
func drawLogo(d display.Display, c color.RGBA) {
	width := int16(logoBitmap[0])<<8 | int16(logoBitmap[1])
	height := int16(logoBitmap[2])<<8 | int16(logoBitmap[3])
	rowBytes := (width + 7) / 8
	screenW, screenH := d.Size()
	left, top := (screenW-width)/2, (screenH-height)/2
	for y := range height {
		for x := range width {
			if logoBitmap[4+int(y)*int(rowBytes)+int(x/8)]&(0x80>>(x%8)) != 0 {
				d.SetPixel(left+x, top+y, c)
			}
		}
	}
}
