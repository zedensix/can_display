//go:build tinygo

package main

import (
	"machine"
	"time"

	"tinygo.org/x/drivers/encoders"
)

// Rotary encoder (KY-040) pins, see docs/wiring.md. GPIO15 must stay
// free: TinyGo borrows it for the RP2040's USB enumeration workaround.
const (
	knobCLK    = machine.GPIO2
	knobDT     = machine.GPIO3
	knobButton = machine.GPIO4
)

// knobLongPress is how long the button is held to reboot.
const knobLongPress = time.Second

// knob reads the encoder and its push button.
type knob struct {
	encoder   *encoders.QuadratureDevice
	position  int
	pressedAt time.Time // zero while released
	longDone  bool
}

func setupKnob() *knob {
	encoder := encoders.NewQuadratureViaInterrupt(knobCLK, knobDT)
	// 4 quadrature steps per detent. If one click moves two steps, the
	// encoder is a half-step one: use 2.
	if err := encoder.Configure(encoders.QuadratureConfig{Precision: 4}); err != nil {
		println("knob:", err.Error())
	}
	// The KY-040 usually leaves its SW pull-up unpopulated.
	knobButton.Configure(machine.PinConfig{Mode: machine.PinInputPullup})
	return &knob{encoder: encoder}
}

// poll returns the detents turned since the last call, whether the
// button was just released after a short press, and whether it has just
// been held for knobLongPress. Called once per frame: sampling every
// 50ms is what debounces the button.
func (k *knob) poll(now time.Time) (detents int, press, long bool) {
	position := k.encoder.Position()
	detents, k.position = position-k.position, position

	down := !knobButton.Get()
	switch {
	case down && k.pressedAt.IsZero():
		k.pressedAt = now
	case down && !k.longDone && now.Sub(k.pressedAt) >= knobLongPress:
		k.longDone, long = true, true
	case !down && !k.pressedAt.IsZero():
		press = !k.longDone
		k.pressedAt, k.longDone = time.Time{}, false
	}
	return detents, press, long
}
