// Package elm327 talks to an ELM327-compatible OBD-II dongle (Vgate
// iCar Pro and friends) over any byte pipe: BLE, WiFi TCP or a UART.
//
// The dongle does the CAN side itself; we only exchange text with it.
// With headers on (ATH1) and spaces off (ATS0), every CAN frame it
// receives is printed as one hex line, e.g. "7E8044105A5" for a coolant
// temp reply. ParseFrameLine turns that back into a canbus.Frame, so
// the rest of the project (Registry, gt86 decoders, UI) can't tell the
// difference between this and a real CAN controller. See Bus.
//
// Transport plumbing is deliberately inverted: the Client writes through
// an io.Writer, and the transport pushes whatever the dongle sends back
// into Client.Receive. That fits BLE (notifications arrive in a
// callback) and is trivial to fake in tests.
package elm327

import (
	"errors"
	"fmt"
	"io"
	"strconv"
	"strings"
	"sync"
	"time"

	"zedensix/can_display/internal/canbus"
)

var (
	// ErrTimeout means the dongle didn't print its ">" prompt in time.
	// Usually the link died (dongle unplugged, BLE dropped).
	ErrTimeout = errors.New("elm327: timed out waiting for prompt")

	// ErrRejected means the dongle answered "?" (unknown command) or
	// something other than OK to a setup command.
	ErrRejected = errors.New("elm327: command rejected")
)

const (
	// DefaultTimeout bounds one request/response round trip. An ECU that
	// doesn't answer yields "NO DATA" well before this (the ELM's own
	// timeout is ~200ms); hitting this means the link is broken.
	DefaultTimeout = 2 * time.Second

	// DefaultResetTimeout is longer: ATZ reboots the dongle's firmware.
	DefaultResetTimeout = 5 * time.Second

	// DefaultRetries is how many times a command is resent after a
	// timeout. Over BLE a notification gets lost now and then: resending
	// is much cheaper than dropping the link.
	DefaultRetries = 2

	rxBuffer = 1024
)

// Client is one ELM327 session. Safe for concurrent use: commands are
// serialized, since the ELM327 only handles one at a time.
type Client struct {
	mu sync.Mutex
	w  io.Writer
	rx chan byte

	// Timeout is how long Command waits for the prompt.
	Timeout time.Duration

	// ResetTimeout is how long Init waits for ATZ to finish.
	ResetTimeout time.Duration

	// Retries is how many times a command is resent after ErrTimeout
	// before giving up.
	Retries int

	// SingleResponse appends the ELM's "expect 1 reply" digit to OBD
	// requests ("010C1" instead of "010C"), so it returns as soon as the
	// engine ECU answers instead of waiting out its timeout for other
	// ECUs. Roughly doubles the poll rate. Turned off automatically if
	// the dongle rejects it.
	SingleResponse bool

	header     uint32
	haveHeader bool
}

// NewClient returns a Client writing to w. Feed the dongle's output to
// Receive, then call Init.
func NewClient(w io.Writer) *Client {
	return &Client{
		w:              w,
		rx:             make(chan byte, rxBuffer),
		Timeout:        DefaultTimeout,
		ResetTimeout:   DefaultResetTimeout,
		Retries:        DefaultRetries,
		SingleResponse: true,
	}
}

// Receive hands bytes from the dongle to the client. Call it from the
// transport's receive path (a BLE notification callback, a TCP read
// loop). Never blocks: bytes are dropped if nobody is reading, which
// only happens if output arrives outside of a command.
func (c *Client) Receive(p []byte) {
	for _, b := range p {
		select {
		case c.rx <- b:
		default:
		}
	}
}

// Init resets the dongle and configures it for this project: echo,
// linefeeds and spaces off, CAN headers on, ISO 15765-4 11bit/500kbps.
func (c *Client) Init() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	// ATZ answers with its version banner, not OK, and echo is still on
	// at that point, so don't check its output.
	if _, err := c.command("ATZ", c.ResetTimeout); err != nil {
		return err
	}
	setup := []string{
		"ATE0", // echo off
		"ATL0", // no linefeeds after CR
		"ATS0", // no spaces between hex bytes
		"ATH1", // print CAN IDs: needed to rebuild frames
		"ATCAF1",
		// Protocol 6 = ISO 15765-4 CAN, 11bit IDs, 500kbps: what the
		// GT86 speaks on its OBD port. Fixing it skips the slow
		// auto-detect. If the dongle refuses to talk to the car, try
		// ATSP0 (auto).
		"ATSP6",
	}
	for _, cmd := range setup {
		if err := c.expectOK(cmd); err != nil {
			return err
		}
	}
	c.haveHeader = false
	return nil
}

// Command sends one raw command (without the trailing CR) and returns
// the non-empty response lines, with echo and "SEARCHING..." removed.
func (c *Client) Command(cmd string) ([]string, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.command(cmd, c.Timeout)
}

// Query sends an OBD request (service byte + PID bytes, no ISO-TP length
// byte, the dongle adds it) to CAN ID header and returns every CAN frame
// printed in reply. An ECU that doesn't answer ("NO DATA") gives zero
// frames and no error.
func (c *Client) Query(header uint32, data []byte) ([]canbus.Frame, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if !c.haveHeader || c.header != header {
		if err := c.expectOK(fmt.Sprintf("ATSH%03X", header)); err != nil {
			return nil, err
		}
		c.header, c.haveHeader = header, true
	}

	req := fmt.Sprintf("%X", data) // two uppercase hex digits per byte
	lines, err := c.command(req+c.responseCountSuffix(), c.Timeout)
	if errors.Is(err, ErrRejected) && c.SingleResponse {
		// Older or cloned firmware doesn't know the count digit.
		c.SingleResponse = false
		lines, err = c.command(req, c.Timeout)
	}
	if err != nil {
		return nil, err
	}

	var frames []canbus.Frame
	for _, l := range lines {
		if f, ok := ParseFrameLine(l); ok {
			frames = append(frames, f)
		}
	}
	return frames, nil
}

// Voltage reads the dongle's supply voltage (ATRV), i.e. what's on OBD
// pin 16: the battery, or the alternator's output with the engine on.
func (c *Client) Voltage() (float32, error) {
	lines, err := c.Command("ATRV")
	if err != nil {
		return 0, err
	}
	for _, l := range lines {
		if v, ok := ParseVoltage(l); ok {
			return v, nil
		}
	}
	return 0, fmt.Errorf("%w: ATRV answered %q", ErrRejected, lines)
}

func (c *Client) responseCountSuffix() string {
	if c.SingleResponse {
		return "1"
	}
	return ""
}

func (c *Client) expectOK(cmd string) error {
	lines, err := c.command(cmd, c.Timeout)
	if err != nil {
		return err
	}
	for _, l := range lines {
		if l == "OK" {
			return nil
		}
	}
	return fmt.Errorf("%w: %s answered %q", ErrRejected, cmd, lines)
}

func (c *Client) command(cmd string, timeout time.Duration) ([]string, error) {
	var raw string
	var err error
	for attempt := 0; attempt <= c.Retries; attempt++ {
		c.drain()
		if _, err := io.WriteString(c.w, cmd+"\r"); err != nil {
			return nil, err
		}
		raw, err = c.readUntilPrompt(timeout)
		if !errors.Is(err, ErrTimeout) {
			break
		}
	}
	if err != nil {
		return nil, err
	}
	lines := SplitResponse(raw, cmd)
	if len(lines) == 1 && lines[0] == "?" {
		return nil, fmt.Errorf("%w: %s", ErrRejected, cmd)
	}
	return lines, nil
}

// drain throws away leftovers from a previous command that timed out,
// so they can't be mistaken for this command's answer.
func (c *Client) drain() {
	for {
		select {
		case <-c.rx:
		default:
			return
		}
	}
}

func (c *Client) readUntilPrompt(timeout time.Duration) (string, error) {
	var buf []byte
	deadline := time.After(timeout)
	for {
		select {
		case b := <-c.rx:
			if b == '>' {
				return string(buf), nil
			}
			buf = append(buf, b)
		case <-deadline:
			return "", ErrTimeout
		}
	}
}

// SplitResponse cuts a raw ELM327 response (everything before the ">"
// prompt) into trimmed, non-empty lines, dropping the echo of cmd and
// the informational "SEARCHING..." line.
func SplitResponse(raw, cmd string) []string {
	raw = strings.ReplaceAll(raw, "\x00", "")
	var lines []string
	for _, l := range strings.FieldsFunc(raw, func(r rune) bool { return r == '\r' || r == '\n' }) {
		l = strings.TrimSpace(l)
		if l == "" || l == cmd || strings.HasPrefix(l, "SEARCHING") {
			continue
		}
		lines = append(lines, l)
	}
	return lines
}

// ParseFrameLine parses one line printed with ATH1 for an 11bit CAN
// frame: 3 hex digits of ID, then up to 8 data bytes, spaces optional
// ("7E8 03 41 05 7B" or "7E80341057B"). Anything else (NO DATA, CAN
// ERROR, 29bit IDs...) returns ok=false.
func ParseFrameLine(line string) (canbus.Frame, bool) {
	s := strings.ReplaceAll(line, " ", "")
	n := len(s) - 3
	if n < 2 || n%2 != 0 || n/2 > 8 {
		return canbus.Frame{}, false
	}
	id, err := strconv.ParseUint(s[:3], 16, 16)
	if err != nil {
		return canbus.Frame{}, false
	}
	f := canbus.Frame{ID: uint32(id), DLC: uint8(n / 2)}
	for i := 0; i < int(f.DLC); i++ {
		b, err := strconv.ParseUint(s[3+2*i:5+2*i], 16, 8)
		if err != nil {
			return canbus.Frame{}, false
		}
		f.Data[i] = byte(b)
	}
	return f, true
}

// ParseVoltage parses an ATRV answer like "12.6V".
func ParseVoltage(line string) (float32, bool) {
	s := strings.TrimSuffix(strings.TrimSpace(line), "V")
	v, err := strconv.ParseFloat(s, 32)
	if err != nil {
		return 0, false
	}
	return float32(v), true
}
