//go:build tinygo

package main

import (
	"device/rp"
	"time"

	"zedensix/can_display/internal/signals"
)

// connectNow in WATCHDOG.SCRATCH0 makes the next boot connect without
// waiting for a click: the user already asked, with a retry. Scratch
// registers survive a watchdog reboot, not a power cut.
const connectNow = 0x86c0ffee

// connectAsked reports whether the previous run rebooted to connect, and
// forgets it.
func connectAsked() bool {
	asked := rp.WATCHDOG.SCRATCH0.Get() == connectNow
	rp.WATCHDOG.SCRATCH0.Set(0)
	return asked
}

// reboot shows status, closes the dongle connection and, after pause,
// resets the RP2040 through the watchdog. With connect, the next boot
// connects right away. A reboot is how this firmware reconnects: a clean
// start is the known-good way back, whatever state the BLE stack is in.
func reboot(state *signals.State, status string, pause time.Duration, connect bool) {
	state.SetStatus(status)
	if dongle != nil {
		if err := dongle.Disconnect(); err != nil {
			logLine("ble: disconnect: " + err.Error())
		}
	}
	time.Sleep(pause)
	if connect {
		rp.WATCHDOG.SCRATCH0.Set(connectNow)
	}
	rp.WATCHDOG.CTRL.SetBits(rp.WATCHDOG_CTRL_TRIGGER)
	select {}
}
