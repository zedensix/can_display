package tests

import (
	"bytes"
	"fmt"
	"strings"
	"testing"

	"zedensix/can_display/internal/flashlog"
)

// fakeFlash behaves like NOR flash: writes can only clear bits.
type fakeFlash struct {
	data []byte
}

func newFakeFlash(size int) *fakeFlash {
	return &fakeFlash{data: bytes.Repeat([]byte{0xFF}, size)}
}

func (f *fakeFlash) ReadAt(p []byte, off int64) (int, error) {
	return copy(p, f.data[off:]), nil
}

func (f *fakeFlash) WriteAt(p []byte, off int64) (int, error) {
	for i, b := range p {
		f.data[off+int64(i)] &= b
	}
	return len(p), nil
}

func (f *fakeFlash) EraseSector(off int64) error {
	copy(f.data[off:off+flashlog.SectorSize], bytes.Repeat([]byte{0xFF}, flashlog.SectorSize))
	return nil
}

func dump(t *testing.T, l *flashlog.Log) string {
	t.Helper()
	var out bytes.Buffer
	if _, err := l.WriteTo(&out); err != nil {
		t.Fatal(err)
	}
	return out.String()
}

func TestFlashLogSurvivesReopen(t *testing.T) {
	flash := newFakeFlash(2 * flashlog.SectorSize)
	l, err := flashlog.Open(flash, int64(len(flash.data)))
	if err != nil {
		t.Fatal(err)
	}
	_ = l.Println("boot")
	_ = l.Println("ble: connected")

	l, err = flashlog.Open(flash, int64(len(flash.data)))
	if err != nil {
		t.Fatal(err)
	}
	_ = l.Println("boot again")

	if got, want := dump(t, l), "boot\nble: connected\nboot again\n"; got != want {
		t.Errorf("log = %q, want %q", got, want)
	}
}

func TestFlashLogWrapsDroppingOldestSector(t *testing.T) {
	flash := newFakeFlash(3 * flashlog.SectorSize)
	l, _ := flashlog.Open(flash, int64(len(flash.data)))
	for i := range 3000 {
		_ = l.Println(fmt.Sprintf("line %d", i))
		if i == 1500 {
			l, _ = flashlog.Open(flash, int64(len(flash.data)))
		}
	}

	lines := strings.Split(strings.TrimSuffix(dump(t, l), "\n"), "\n")
	// The first line can be cut: its start went with the dropped sector.
	lines = lines[1:]
	if lines[len(lines)-1] != "line 2999" {
		t.Fatalf("last line = %q, want line 2999", lines[len(lines)-1])
	}
	for i := 1; i < len(lines); i++ {
		var previous, current int
		_, _ = fmt.Sscanf(lines[i-1], "line %d", &previous)
		_, _ = fmt.Sscanf(lines[i], "line %d", &current)
		if current != previous+1 {
			t.Fatalf("line %d is %q after %q, want consecutive lines", i, lines[i], lines[i-1])
		}
	}
	if len(lines) >= 3000 {
		t.Errorf("kept %d lines in 3 sectors, the oldest should have been dropped", len(lines))
	}
}

func TestFlashLogErasesForeignData(t *testing.T) {
	flash := newFakeFlash(2 * flashlog.SectorSize)
	for i := range flash.data {
		flash.data[i] = byte(i % 251)
	}
	l, err := flashlog.Open(flash, int64(len(flash.data)))
	if err != nil {
		t.Fatal(err)
	}
	_ = l.Println("boot")
	if got := dump(t, l); got != "boot\n" {
		t.Errorf("log = %q, want only the new line", got)
	}
}
