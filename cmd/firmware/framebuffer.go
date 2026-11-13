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
	panel *st7789.Device
	image pixel.Image[pixel.RGB565BE]
}

func newFrameBuffer(panel *st7789.Device) *frameBuffer {
	width, height := panel.Size()
	return &frameBuffer{panel: panel, image: pixel.NewImage[pixel.RGB565BE](int(width), int(height))}
}

func (f *frameBuffer) Size() (int16, int16) {
	return f.panel.Size()
}

func (f *frameBuffer) SetPixel(x, y int16, c color.RGBA) {
	width, height := f.panel.Size()
	if x < 0 || y < 0 || x >= width || y >= height {
		return
	}
	f.image.Set(int(x), int(y), pixel.NewColor[pixel.RGB565BE](c.R, c.G, c.B))
}

func (f *frameBuffer) Display() error {
	return f.panel.DrawBitmap(0, 0, f.image)
}
