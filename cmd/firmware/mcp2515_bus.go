//go:build tinygo

package main

import (
	"time"

	"tinygo.org/x/drivers/mcp2515"

	"zedensix/can_display/internal/canbus"
)

// mcp2515Bus adapts an *mcp2515.Device -- which exposes polling methods,
// Received() and Rx() -- to canbus.Bus's blocking ReadFrame(), so the
// same canbus.Registry.Run loop works regardless of what kind of bus
// it's reading from. It also implements canbus.Writer (WriteFrame),
// needed for the OBD-II request/response polling in
// internal/canbus/obd2 -- the passive decoders never use that side.
type mcp2515Bus struct {
	dev *mcp2515.Device
}

// pollInterval is how often we check the controller for a new message
// while waiting. The MCP2515's INT pin could be wired up for a
// zero-latency interrupt-driven version of this instead; polling is
// simpler and fast enough for dashboard-refresh-rate data.
const pollInterval = time.Millisecond

func (b mcp2515Bus) ReadFrame() (canbus.Frame, error) {
	for !b.dev.Received() {
		time.Sleep(pollInterval)
	}
	msg, err := b.dev.Rx()
	if err != nil {
		return canbus.Frame{}, err
	}
	f := canbus.Frame{ID: msg.ID, DLC: msg.Dlc}
	copy(f.Data[:], msg.Data)
	return f, nil
}

func (b mcp2515Bus) WriteFrame(f canbus.Frame) error {
	return b.dev.Tx(f.ID, f.DLC, f.Data[:f.DLC])
}
