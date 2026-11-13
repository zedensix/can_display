//go:build tinygo

package main

import (
	"image/color"

	"tinygo.org/x/drivers/pixel"
	"tinygo.org/x/drivers/st7789"
)

// frameBuffer draws in RAM and sends the whole frame in one SPI burst on
// Display. The ST7789 driver has no buffer: drawing on it directly costs
// several SPI commands per pixel, and the redraw visibly sweeps the panel.
type frameBuffer struct {
	panel         *st7789.Device
	image         pixel.Image[pixel.RGB565BE]
	raw           []byte
	width, height int16
}

func newFrameBuffer(panel *st7789.Device) *frameBuffer {
	width, height := panel.Size()
	image := pixel.NewImage[pixel.RGB565BE](int(width), int(height))
	return &frameBuffer{panel: panel, image: image, raw: image.RawBuffer(), width: width, height: height}
}

func (f *frameBuffer) Size() (int16, int16) {
	return f.width, f.height
}

func (f *frameBuffer) SetPixel(x, y int16, c color.RGBA) {
	if x < 0 || y < 0 || x >= f.width || y >= f.height {
		return
	}
	high, low := rgb565(c)
	i := (int(y)*int(f.width) + int(x)) * 2
	f.raw[i], f.raw[i+1] = high, low
}

// FillRectangle is the fast path internal/ui takes for solid areas: it
// paints the first row, then copies it down.
func (f *frameBuffer) FillRectangle(x, y, width, height int16, c color.RGBA) error {
	x0, y0 := max(x, 0), max(y, 0)
	x1, y1 := min(x+width, f.width), min(y+height, f.height)
	if x0 >= x1 || y0 >= y1 {
		return nil
	}
	high, low := rgb565(c)
	rowStart := func(row int16) int { return (int(row)*int(f.width) + int(x0)) * 2 }
	rowLen := int(x1-x0) * 2

	first := f.raw[rowStart(y0) : rowStart(y0)+rowLen]
	for i := 0; i < rowLen; i += 2 {
		first[i], first[i+1] = high, low
	}
	for row := y0 + 1; row < y1; row++ {
		copy(f.raw[rowStart(row):rowStart(row)+rowLen], first)
	}
	return nil
}

func (f *frameBuffer) Display() error {
	return f.panel.DrawBitmap(0, 0, f.image)
}

// rgb565 packs c the way the panel reads it: 5 bits red, 6 green, 5
// blue, big-endian.
func rgb565(c color.RGBA) (high, low byte) {
	v := uint16(c.R&0xF8)<<8 | uint16(c.G&0xFC)<<3 | uint16(c.B)>>3
	return byte(v >> 8), byte(v)
}
