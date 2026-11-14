//go:build tinygo

package main

import (
	"time"

	"zedensix/can_display/internal/signals"
)

// recoverCrash, deferred at the top of every goroutine this firmware
// starts, writes a panic to the flash log and reboots. Without it the
// message only reaches the serial console, and the watchdog reboots 8s
// later without a trace. Panics in the bluetooth package's own
// goroutines aren't caught: those still end with the watchdog.
func recoverCrash(state *signals.State) {
	r := recover()
	if r == nil {
		return
	}
	message := "unknown panic"
	switch v := r.(type) {
	case error:
		message = v.Error()
	case string:
		message = v
	}
	logLine("panic: " + message)
	reboot(state, "CRASH\n"+message+"\nrebooting...", 5*time.Second, false)
}
