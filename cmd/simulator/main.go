// Command simulator opens a desktop window that renders the exact same
// dashboard UI the Pico W firmware draws to its ST7789 panel, fed by
// fake data instead of a real CAN bus. It's meant to make iterating on
// internal/ui fast: no flashing, no car required.
package main

import (
	"image"
	"image/color"
	"log"
	"time"

	"github.com/hajimehoshi/ebiten/v2"

	"zedensix/can_display/internal/signals"
	"zedensix/can_display/internal/sim"
	"zedensix/can_display/internal/ui"
)

// panelWidth/panelHeight match the real ST7789 2.0" 240x320 panel this
// project targets (see /docs/wiring.md). Change these if you use a
// different size display -- the UI layout adapts automatically.
const (
	panelWidth  = 240
	panelHeight = 320
	// windowScale blows the panel up on-screen so it's actually visible
	// on a modern desktop display; it has no effect on what would be
	// drawn to the real panel.
	windowScale = 3
)

// framebuffer is a software Display backed by a plain image.RGBA. It
// satisfies display.Display (== drivers.Displayer), so it's a drop-in
// stand-in for the real st7789.Device the firmware uses -- the same
// internal/ui code renders to both without modification.
type framebuffer struct {
	img *image.RGBA
}

func newFramebuffer(w, h int) *framebuffer {
	return &framebuffer{img: image.NewRGBA(image.Rect(0, 0, w, h))}
}

func (f *framebuffer) Size() (int16, int16) { return panelWidth, panelHeight }

func (f *framebuffer) SetPixel(x, y int16, c color.RGBA) {
	if x < 0 || y < 0 || int(x) >= panelWidth || int(y) >= panelHeight {
		return
	}
	f.img.SetRGBA(int(x), int(y), c)
}

// Display is a no-op here: there's no separate panel buffer to flush,
// SetPixel already wrote straight into f.img. The real ST7789 driver's
// Display() is what actually pushes pixels over SPI on hardware.
func (f *framebuffer) Display() error { return nil }

type game struct {
	fb        *framebuffer
	screenImg *ebiten.Image
	gen       *sim.Generator
	state     *signals.State
	dash      ui.Dashboard
	start     time.Time
}

func (g *game) Update() error {
	g.gen.Tick(g.state, time.Now())
	return nil
}

func (g *game) Draw(screen *ebiten.Image) {
	running, err := g.dash.RenderBoot(g.fb, time.Since(g.start))
	if !running {
		err = g.dash.Render(g.fb, g.state.Snapshot())
	}
	if err != nil {
		log.Println("render:", err)
		return
	}
	g.screenImg.WritePixels(g.fb.img.Pix)

	op := &ebiten.DrawImageOptions{}
	op.GeoM.Scale(windowScale, windowScale)
	op.Filter = ebiten.FilterNearest // keep pixel edges crisp, like the real panel
	screen.DrawImage(g.screenImg, op)
}

func (g *game) Layout(outsideWidth, outsideHeight int) (int, int) {
	return panelWidth * windowScale, panelHeight * windowScale
}

func main() {
	gen := sim.NewGenerator()
	// Ethanol % has a real data source in the firmware now (the Pico ADC +
	// flex-fuel sensor, see internal/sensors), so it's enabled here too
	// for parity. Battery/AFR are still preview-only: flip them off to
	// see exactly what the firmware can show today.
	gen.EnableBattery = true
	gen.EnableAFR = true
	gen.EnableEthanol = true

	g := &game{
		fb:        newFramebuffer(panelWidth, panelHeight),
		screenImg: ebiten.NewImage(panelWidth, panelHeight),
		gen:       gen,
		state:     &signals.State{},
		dash: ui.Dashboard{
			Theme: ui.Dark,
			Tiles: ui.DefaultTiles(),
		},
		start: time.Now(),
	}

	ebiten.SetWindowSize(panelWidth*windowScale, panelHeight*windowScale)
	ebiten.SetWindowTitle("GT86 Dash Simulator")
	if err := ebiten.RunGame(g); err != nil {
		log.Fatal(err)
	}
}
