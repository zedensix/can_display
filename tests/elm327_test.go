package tests

import (
	"errors"
	"strings"
	"sync"
	"testing"
	"time"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/gt86"
	"zedensix/can_display/internal/canbus/obd2"
	"zedensix/can_display/internal/elm327"
	"zedensix/can_display/internal/signals"
)

// fakeELM plays the dongle: every CR-terminated command written to it
// gets the scripted answer (or "?" if unknown) pushed back into the
// client, followed by the prompt, like a real ELM327 would.
type fakeELM struct {
	mu      sync.Mutex
	client  *elm327.Client
	answers map[string]string
	got     []string
	pending string
	// drop is how many commands to leave unanswered before answering,
	// like a BLE notification lost on the way back.
	drop int
}

func newFakeELM(answers map[string]string) (*fakeELM, *elm327.Client) {
	f := &fakeELM{answers: answers}
	f.client = elm327.NewClient(f)
	f.client.Timeout = 200 * time.Millisecond
	f.client.ResetTimeout = 200 * time.Millisecond
	return f, f.client
}

func (f *fakeELM) Write(p []byte) (int, error) {
	f.mu.Lock()
	f.pending += string(p)
	var cmds []string
	for {
		i := strings.IndexByte(f.pending, '\r')
		if i < 0 {
			break
		}
		cmds = append(cmds, f.pending[:i])
		f.pending = f.pending[i+1:]
	}
	f.got = append(f.got, cmds...)
	f.mu.Unlock()

	for _, cmd := range cmds {
		f.mu.Lock()
		dropped := f.drop > 0
		if dropped {
			f.drop--
		}
		f.mu.Unlock()
		if dropped {
			continue
		}
		ans, ok := f.answers[cmd]
		if !ok {
			ans = "?"
		}
		if ans == "<silence>" {
			continue
		}
		f.client.Receive([]byte(ans + "\r\r>"))
	}
	return len(p), nil
}

func (f *fakeELM) commands() []string {
	f.mu.Lock()
	defer f.mu.Unlock()
	return append([]string(nil), f.got...)
}

var initAnswers = map[string]string{
	"ATZ":     "ATZ\r\rELM327 v2.3",
	"ATE0":    "ATE0\rOK",
	"ATL0":    "OK",
	"ATS0":    "OK",
	"ATH1":    "OK",
	"ATCAF1":  "OK",
	"ATSP6":   "OK",
	"ATSH7DF": "OK",
}

func TestELMInit(t *testing.T) {
	fake, client := newFakeELM(initAnswers)
	if err := client.Init(); err != nil {
		t.Fatalf("Init: %v", err)
	}
	want := []string{"ATZ", "ATE0", "ATL0", "ATS0", "ATH1", "ATCAF1", "ATSP6"}
	if got := fake.commands(); strings.Join(got, ",") != strings.Join(want, ",") {
		t.Errorf("commands = %v, want %v", got, want)
	}
}

func TestELMInitFailsOnRejectedSetup(t *testing.T) {
	answers := map[string]string{}
	for k, v := range initAnswers {
		answers[k] = v
	}
	answers["ATSP6"] = "?"
	_, client := newFakeELM(answers)
	if err := client.Init(); !errors.Is(err, elm327.ErrRejected) {
		t.Fatalf("Init err = %v, want ErrRejected", err)
	}
}

func TestELMTimeout(t *testing.T) {
	fake, client := newFakeELM(map[string]string{"ATZ": "<silence>"})
	if err := client.Init(); !errors.Is(err, elm327.ErrTimeout) {
		t.Fatalf("Init err = %v, want ErrTimeout", err)
	}
	if got, want := len(fake.commands()), 1+client.Retries; got != want {
		t.Errorf("ATZ sent %d times, want %d (first try + retries)", got, want)
	}
}

func TestELMRetriesAfterLostAnswer(t *testing.T) {
	fake, client := newFakeELM(map[string]string{"ATRV": "12.6V"})
	fake.drop = 1
	v, err := client.Voltage()
	if err != nil {
		t.Fatalf("Voltage err = %v, want the retry to succeed", err)
	}
	if v != 12.6 {
		t.Errorf("Voltage = %v, want 12.6", v)
	}
	if got := fake.commands(); len(got) != 2 {
		t.Errorf("commands = %q, want ATRV sent twice", got)
	}
}

func TestParseFrameLine(t *testing.T) {
	cases := []struct {
		line string
		ok   bool
		want canbus.Frame
	}{
		{"7E8044105A5", true, canbus.Frame{ID: 0x7E8, DLC: 4, Data: [8]byte{0x04, 0x41, 0x05, 0xA5}}},
		{"7E8 04 41 05 A5", true, canbus.Frame{ID: 0x7E8, DLC: 4, Data: [8]byte{0x04, 0x41, 0x05, 0xA5}}},
		{"7E80641240800000000", true, canbus.Frame{ID: 0x7E8, DLC: 8, Data: [8]byte{0x06, 0x41, 0x24, 0x08, 0, 0, 0, 0}}},
		{"NO DATA", false, canbus.Frame{}},
		{"CAN ERROR", false, canbus.Frame{}},
		{"7E8", false, canbus.Frame{}},
		{"7E80", false, canbus.Frame{}},
		{"7E8064124080000000000", false, canbus.Frame{}}, // 9 bytes
	}
	for _, c := range cases {
		got, ok := elm327.ParseFrameLine(c.line)
		if ok != c.ok || (ok && got != c.want) {
			t.Errorf("ParseFrameLine(%q) = %v, %v; want %v, %v", c.line, got, ok, c.want, c.ok)
		}
	}
}

func TestParseVoltage(t *testing.T) {
	if v, ok := elm327.ParseVoltage("12.6V"); !ok || v < 12.59 || v > 12.61 {
		t.Errorf("ParseVoltage(12.6V) = %v, %v", v, ok)
	}
	if _, ok := elm327.ParseVoltage("NO DATA"); ok {
		t.Error("ParseVoltage(NO DATA) should fail")
	}
}

// End to end: the OBD poller writes through the ELM bus, the replies
// come back through the same Registry the MCP2515 path uses, and land
// in State.
func TestELMBusFeedsGT86Decoders(t *testing.T) {
	answers := map[string]string{
		"ATSH7DF": "OK",
		"010C1":   "7E804410C1AF8", // 0x1AF8/4 = 1726 RPM
		"01051":   "7E80341055A",   // 0x5A-40 = 50 degC
	}
	_, client := newFakeELM(answers)
	bus := elm327.NewBus(client)

	state := &signals.State{}
	reg := canbus.NewRegistry()
	gt86.Register(reg, state)
	gt86.RegisterOBD(reg, state)

	for _, pid := range []byte{obd2.PIDEngineRPM, obd2.PIDCoolantTemp} {
		if err := bus.WriteFrame(obd2.RequestFrame(obd2.Request{Mode: obd2.ModeCurrentData, PID: pid})); err != nil {
			t.Fatalf("WriteFrame(%#x): %v", pid, err)
		}
		f, _ := bus.ReadFrame()
		reg.Dispatch(f, time.Now())
	}

	snap := state.Snapshot()
	if !snap.RPM.Valid || snap.RPM.Value != 1726 {
		t.Errorf("RPM = %+v, want 1726", snap.RPM)
	}
	if !snap.CoolantTempC.Valid || snap.CoolantTempC.Value != 50 {
		t.Errorf("coolant = %+v, want 50", snap.CoolantTempC)
	}
}

func TestELMFallsBackWithoutResponseCount(t *testing.T) {
	answers := map[string]string{
		"ATSH7DF": "OK",
		"010C":    "7E804410C0FA0", // 1000 RPM
	}
	fake, client := newFakeELM(answers)
	frames, err := client.Query(obd2.FunctionalRequestID, []byte{0x01, 0x0C})
	if err != nil {
		t.Fatalf("Query: %v", err)
	}
	if len(frames) != 1 || frames[0].ID != obd2.ResponseIDECU1 {
		t.Fatalf("frames = %v", frames)
	}
	if client.SingleResponse {
		t.Error("SingleResponse should be disabled after the dongle rejected it")
	}
	if got := fake.commands(); strings.Join(got, ",") != "ATSH7DF,010C1,010C" {
		t.Errorf("commands = %v", got)
	}
}

func TestELMNoDataIsNotAnError(t *testing.T) {
	_, client := newFakeELM(map[string]string{"ATSH7DF": "OK", "01241": "NO DATA"})
	frames, err := client.Query(obd2.FunctionalRequestID, []byte{0x01, 0x24})
	if err != nil || len(frames) != 0 {
		t.Fatalf("Query = %v, %v; want no frames, no error", frames, err)
	}
}

func TestELMBusRejectsMultiFrame(t *testing.T) {
	_, client := newFakeELM(nil)
	bus := elm327.NewBus(client)
	err := bus.WriteFrame(canbus.Frame{ID: 0x7DF, DLC: 8, Data: [8]byte{0x10, 0x09}})
	if !errors.Is(err, elm327.ErrUnsupportedFrame) {
		t.Fatalf("err = %v, want ErrUnsupportedFrame", err)
	}
}

func TestOBDDecoders(t *testing.T) {
	if v, ok := obd2.EngineRPM([]byte{0x1A, 0xF8}); !ok || v != 1726 {
		t.Errorf("EngineRPM = %v, %v", v, ok)
	}
	if v, ok := obd2.CoolantTempC([]byte{0x5A}); !ok || v != 50 {
		t.Errorf("CoolantTempC = %v, %v", v, ok)
	}
	if _, ok := obd2.EngineRPM([]byte{0x1A}); ok {
		t.Error("EngineRPM should reject a short payload")
	}
}
