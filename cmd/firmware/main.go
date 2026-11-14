//go:build tinygo

// Command firmware is the Raspberry Pi Pico W program: it reads the
// GT86's data either through an ELM327 Bluetooth LE OBD dongle
// (elm_ble.go) or straight off the CAN bus through an MCP2515 SPI
// controller (see dataSource), reads the flex-fuel sensor through the
// RP2040's own ADC, and renders internal/ui's dashboard to an ST7789
// SPI display. It shares every package under internal/ with
// cmd/simulator -- only this file and the pin wiring are specific to
// real hardware.
//
// Build with TinyGo, not `go build` (this file needs the `machine`
// package, which only exists under the TinyGo compiler). See the
// firmware task in .mise/tasks for the exact command.
package main

import (
	"device/rp"
	"machine"
	"time"

	"tinygo.org/x/drivers/mcp2515"
	"tinygo.org/x/drivers/st7789"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/gt86"
	"zedensix/can_display/internal/sensors"
	"zedensix/can_display/internal/signals"
	"zedensix/can_display/internal/sim"
	"zedensix/can_display/internal/ui"
)

// Pin wiring -- adjust these to match your actual breadboard/PCB layout;
// see /docs/wiring.md for the reference wiring this project assumes.
//
// The RP2040 has two SPI peripherals: the display gets SPI0, the MCP2515
// gets SPI1. GPIO23/24/25/29 are taken by the Pico W's wireless chip.
const (
	displaySCK   = machine.GPIO18
	displaySDO   = machine.GPIO19 // MOSI; display is write-only, no MISO needed
	displayCS    = machine.GPIO17
	displayDC    = machine.GPIO20
	displayReset = machine.GPIO21
	displayBL    = machine.GPIO22

	canSCK = machine.GPIO10
	canSDO = machine.GPIO11 // MOSI
	canSDI = machine.GPIO12 // MISO
	canCS  = machine.GPIO13

	// ethanolPin is ADC0, fed through the divider in /docs/wiring.md.
	ethanolPin = machine.ADC0
)

// dataSource picks where car data comes from.
type dataSource int

const (
	// sourceELM327BLE talks to an ELM327 OBD dongle over Bluetooth LE.
	// No wiring to the car beyond the dongle itself, but only standard
	// OBD PIDs at a few Hz: RPM, coolant, AFR, battery voltage. No oil
	// temperature. See elm_ble.go.
	sourceELM327BLE dataSource = iota

	// sourceMCP2515 listens to the CAN bus directly through an MCP2515
	// wired to the OBD port. Faster, and gets oil temp from the
	// broadcast frames. See docs/wiring.md.
	sourceMCP2515
)

const source = sourceELM327BLE

// simulate replaces source with internal/sim's fake data, to check the
// display and the UI on the real panel without a car. Set by the `sim`
// build tag (sim.go).
var simulate bool

// adcVref is the RP2040 ADC reference (the 3.3V rail on a stock Pico W).
// Tune it against a multimeter: the onboard reference is noisy and
// typically reads a few tens of mV off.
const adcVref float32 = 3.3

// panelWidth/panelHeight must match cmd/simulator's constants -- both
// feed the same internal/ui.Dashboard, which lays itself out to
// whatever size it's given.
const (
	panelWidth  = 240
	panelHeight = 320
)

// tickInterval is the redraw (and ethanol ADC read) period: 20 frames a
// second. A frame is ~20ms of SPI transfer plus the drawing, so this
// leaves the rest of the time to the data source goroutines.
const tickInterval = 50 * time.Millisecond

// minYield is the least time each frame leaves to the other goroutines.
const minYield = 10 * time.Millisecond

func main() {
	openLog()
	connectRetry := connectAsked()
	if rp.WATCHDOG.REASON.HasBits(rp.WATCHDOG_REASON_TIMER) {
		logLine("boot: the watchdog reset the board, the previous run froze")
	} else {
		logLine("boot")
	}
	// A panic or a stuck goroutine freezes TinyGo instead of rebooting;
	// the watchdog turns any freeze into a reset.
	machine.Watchdog.Configure(machine.WatchdogConfig{TimeoutMillis: machine.WatchdogMaxTimeout})
	machine.Watchdog.Start()

	panel := setupDisplay()
	frame := newFrameBuffer(&panel)
	cfg := loadSettings()
	savedCfg, appliedCfg := cfg, cfg
	setBrightness := setupBacklight()
	applySettings(&panel, setBrightness, cfg)
	ethanolADC := setupEthanolADC()

	state := &signals.State{}
	defer recoverCrash(state)
	reg := canbus.NewRegistry()
	gt86.Register(reg, state)

	var generator *sim.Generator
	switch {
	case simulate:
		generator = sim.NewGenerator()
		generator.EnableAFR = true
		generator.EnableEthanol = true
		generator.EnableBattery = true
	case source == sourceELM327BLE:
		gt86.RegisterOBD(reg, state)
		// Connecting takes seconds (scan, connect, dongle reset), so it
		// runs in the background: the dash draws "--" until data flows.
		go func() {
			defer recoverCrash(state)
			runELM327BLE(reg, state, cfg.AutoConnect || connectRetry)
		}()
	case source == sourceMCP2515:
		runMCP2515(reg, state)
	}

	dash := ui.Dashboard{Theme: ui.Dark, Tiles: ui.DefaultTiles()}
	boot := time.Now()
	for {
		machine.Watchdog.Update()
		elapsed := time.Since(boot)
		if elapsed >= ui.BootDuration {
			break
		}
		err := frame.draw(func() error {
			_, err := dash.RenderBoot(frame, elapsed)
			return err
		})
		if err != nil {
			println("render error:", err.Error())
		}
		// Lets the data source goroutine run meanwhile: the scan starts
		// during the animation, not after it.
		time.Sleep(10 * time.Millisecond)
	}

	knob := setupKnob()
	var screen ui.Screen
	for {
		frameStart := time.Now()
		machine.Watchdog.Update()
		serveLog()
		if generator != nil {
			generator.Tick(state, frameStart)
		} else {
			updateEthanolPercent(ethanolADC, state)
		}

		detents, press, long := knob.poll(frameStart)
		screen.Turn(detents, len(dash.Tiles), &cfg)
		// On the gauges, a waiting connection prompt takes the click.
		if press && !(screen.Page == ui.PageGauges && answerPrompt()) {
			screen.Press(&cfg)
		}
		if long {
			logLine("knob: reboot")
			// In a goroutine so the main loop keeps drawing meanwhile.
			go reboot(state, "rebooting...", 500*time.Millisecond, false)
		}
		if cfg != appliedCfg {
			applySettings(&panel, setBrightness, cfg)
			appliedCfg = cfg
		}
		if screen.Page == ui.PageGauges && cfg != savedCfg {
			saveSettings(cfg)
			savedCfg = cfg
		}

		snap := state.Snapshot()
		if err := frame.draw(func() error { return dash.Render(frame, snap, screen, cfg) }); err != nil {
			println("render error:", err.Error())
		}
		// Sleep is the only place the main loop lets the BLE and OBD
		// goroutines run (the scheduler is cooperative), and TinyGo's
		// Sleep doesn't yield at all for a zero or negative duration: a
		// frame slower than tickInterval would starve them for good.
		time.Sleep(max(tickInterval-time.Since(frameStart), minYield))
	}
}

// runMCP2515 starts the direct CAN path: passive listening plus the
// AFR OBD request, both through the MCP2515.
func runMCP2515(reg *canbus.Registry, state *signals.State) {
	bus := mcp2515Bus{dev: setupCAN()}

	// The CAN read loop runs forever in its own goroutine; the main
	// goroutine polls the ADC and redraws on a timer.
	go func() {
		defer recoverCrash(state)
		err := reg.Run(bus)
		// Run only returns on a read error, which means the MCP2515 (or
		// its SPI link) is in a bad state -- crash loudly rather than
		// silently freezing the dash with stale numbers.
		panic("CAN read loop exited: " + err.Error())
	}()

	// AFR needs an active request/response, not passive listening --
	// see internal/canbus/gt86's afr.go for why, and for how
	// speculative this one is. Its response comes back through reg
	// above like any other frame.
	go func() {
		defer recoverCrash(state)
		if err := gt86.AFRPoller(bus).Run(); err != nil {
			panic("AFR poll loop exited: " + err.Error())
		}
	}()
}

// updateEthanolPercent reads the flex-fuel sensor through the ADC,
// undoes the resistor divider, and converts the result to a percentage.
// See internal/sensors for the conversion math and its caveats.
func updateEthanolPercent(adc machine.ADC, state *signals.State) {
	adcVolts := sensors.RawToVolts(adc.Get(), adcVref)
	sensorVolts := sensors.EthanolDivider.Undo(adcVolts)
	state.SetEthanolPercent(sensors.EthanolPercent(sensorVolts), time.Now())
}

func setupDisplay() st7789.Device {
	spi := machine.SPI0
	err := spi.Configure(machine.SPIConfig{
		// 62.5MHz is the ST7789's write clock limit and the RP2040's
		// fastest SPI clock at 125MHz. Garbled pixels on long or loose
		// wires: drop to 31.25e6.
		Frequency: 62.5e6,
		SCK:       displaySCK,
		SDO:       displaySDO,
		SDI:       machine.NoPin,
	})
	if err != nil {
		panic("display SPI configure: " + err.Error())
	}

	dev := st7789.New(spi, displayReset, displayDC, displayCS, displayBL)
	dev.Configure(st7789.Config{
		Width:  panelWidth,
		Height: panelHeight,
	})
	return dev
}

// setupCAN brings up the MCP2515 on SPI1. 1MHz is conservative on
// purpose: it's plenty for dashboard-rate data and survives cheap level
// shifters and long jumper wires.
func setupCAN() *mcp2515.Device {
	spi := machine.SPI1
	err := spi.Configure(machine.SPIConfig{
		Frequency: 1e6,
		SCK:       canSCK,
		SDO:       canSDO,
		SDI:       canSDI,
	})
	if err != nil {
		panic("CAN SPI configure: " + err.Error())
	}

	dev := mcp2515.New(spi, canCS)
	dev.Configure(mcp2515.Configuration{Extended: false})

	// Most cheap MCP2515 breakout boards ship with a 16MHz crystal; if
	// yours uses an 8MHz crystal, change Clock16MHz to Clock8MHz.
	// 500kbps matches the GT86's factory high-speed CAN bus.
	if err := dev.Begin(mcp2515.CAN500kBps, mcp2515.Clock16MHz); err != nil {
		panic("CAN begin: " + err.Error())
	}
	return dev
}

func setupEthanolADC() machine.ADC {
	machine.InitADC()
	adc := machine.ADC{Pin: ethanolPin}
	adc.Configure(machine.ADCConfig{})
	return adc
}
