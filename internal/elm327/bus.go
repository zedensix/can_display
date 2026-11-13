package elm327

import (
	"errors"

	"zedensix/can_display/internal/canbus"
)

// ErrUnsupportedFrame is returned by Bus.WriteFrame for anything that
// isn't an ISO-TP single frame: the ELM327 builds the framing itself, so
// only short requests can be passed through.
var ErrUnsupportedFrame = errors.New("elm327: only ISO-TP single-frame requests can be sent")

// Bus makes a Client look like a CAN controller: it implements
// canbus.Bus and canbus.Writer, so an obd2.Poller can write to it and a
// canbus.Registry can read the answers, exactly as with the MCP2515.
//
// The difference: WriteFrame blocks until the dongle has answered, and
// the frames it printed are queued for ReadFrame. Nothing is ever
// received unasked, so the passive gt86 decoders (0x140, 0x360) never
// fire on this bus.
type Bus struct {
	Client *Client
	frames chan canbus.Frame
}

// NewBus wraps an initialized Client.
func NewBus(c *Client) *Bus {
	return &Bus{Client: c, frames: make(chan canbus.Frame, 16)}
}

// WriteFrame sends f's ISO-TP single-frame payload to f.ID and queues
// the replies. Replies are dropped if nobody runs ReadFrame.
func (b *Bus) WriteFrame(f canbus.Frame) error {
	pci := f.Data[0]
	n := int(pci & 0x0F)
	if pci&0xF0 != 0 || n == 0 || n > 7 || int(f.DLC) < 1+n {
		return ErrUnsupportedFrame
	}
	replies, err := b.Client.Query(f.ID, f.Data[1:1+n])
	if err != nil {
		return err
	}
	for _, r := range replies {
		select {
		case b.frames <- r:
		default:
		}
	}
	return nil
}

// ReadFrame blocks until a reply frame is available. It never fails:
// link errors surface on WriteFrame instead.
func (b *Bus) ReadFrame() (canbus.Frame, error) {
	return <-b.frames, nil
}
