// Package flashlog keeps a short text log in a ring of flash sectors, so
// what happened in the car can be read back at the desk. Text never
// contains 0xFF, the value of erased flash: the first erased byte after
// a written one is where the previous run stopped.
package flashlog

import (
	"errors"
	"io"
	"strings"
	"sync"
)

const (
	// SectorSize is the flash erase unit.
	SectorSize = 4096
	// PageSize is the flash program unit.
	PageSize = 256

	erased = 0xFF
)

// Device is the flash region holding the log, offsets counted from its
// start. Like NOR flash, WriteAt can only clear bits, and EraseSector
// sets a whole sector back to 0xFF.
type Device interface {
	ReadAt(p []byte, off int64) (int, error)
	WriteAt(p []byte, off int64) (int, error)
	EraseSector(off int64) error
}

// Log appends lines to a Device, overwriting the oldest sector once full.
type Log struct {
	mu   sync.Mutex
	dev  Device
	size int64
	pos  int64
}

var errSize = errors.New("flashlog: size must be at least two whole sectors")

// Open picks the log up where the previous run left it. A region that
// doesn't hold a log (another firmware's leftovers) is erased.
func Open(dev Device, size int64) (*Log, error) {
	if size < 2*SectorSize || size%SectorSize != 0 {
		return nil, errSize
	}
	l := &Log{dev: dev, size: size}
	pos, ok, err := l.findEnd()
	if err != nil {
		return nil, err
	}
	if !ok {
		for off := int64(0); off < size; off += SectorSize {
			if err := dev.EraseSector(off); err != nil {
				return nil, err
			}
		}
		pos = 0
	}
	l.pos = pos
	return l, nil
}

// findEnd returns the first erased byte that follows a written one, and
// whether the rest of its sector is blank, as writes need it to be.
func (l *Log) findEnd() (int64, bool, error) {
	var last [1]byte
	if _, err := l.dev.ReadAt(last[:], l.size-1); err != nil {
		return 0, false, err
	}
	prev := last[0]
	end := int64(-1)
	allErased := true
	var page [PageSize]byte
	for off := int64(0); off < l.size; off += PageSize {
		if _, err := l.dev.ReadAt(page[:], off); err != nil {
			return 0, false, err
		}
		for i, b := range page {
			if b != erased {
				allErased = false
			} else if prev != erased && end < 0 {
				end = off + int64(i)
			}
			prev = b
		}
	}
	if allErased {
		return 0, true, nil
	}
	if end < 0 {
		return 0, false, nil
	}
	blank, err := l.blank(end, (end/SectorSize+1)*SectorSize)
	return end, blank, err
}

func (l *Log) blank(from, to int64) (bool, error) {
	var b [1]byte
	for off := from; off < to; off++ {
		if _, err := l.dev.ReadAt(b[:], off); err != nil {
			return false, err
		}
		if b[0] != erased {
			return false, nil
		}
	}
	return true, nil
}

// Println appends line and a newline. Each flash page it touches is
// read and written back whole, as the flash only programs whole pages.
func (l *Log) Println(line string) error {
	l.mu.Lock()
	defer l.mu.Unlock()
	data := []byte(strings.ReplaceAll(line, "\xff", "?") + "\n")
	var page [PageSize]byte
	for len(data) > 0 {
		pageStart := l.pos / PageSize * PageSize
		if _, err := l.dev.ReadAt(page[:], pageStart); err != nil {
			return err
		}
		n := copy(page[l.pos-pageStart:], data)
		if _, err := l.dev.WriteAt(page[:], pageStart); err != nil {
			return err
		}
		data = data[n:]
		l.pos = (l.pos + int64(n)) % l.size
		// Erasing the next sector as soon as it's reached keeps a blank
		// gap after the newest line, which Open looks for.
		if l.pos%SectorSize == 0 {
			if err := l.dev.EraseSector(l.pos); err != nil {
				return err
			}
		}
	}
	return nil
}

// WriteTo writes the whole log to w, oldest line first. Once the log has
// wrapped, the first line can be cut: its start went with the sector
// overwritten last.
func (l *Log) WriteTo(w io.Writer) (int64, error) {
	l.mu.Lock()
	defer l.mu.Unlock()
	// The rest of the newest sector is blank, so the oldest line starts
	// at the next sector.
	start := (l.pos/SectorSize + 1) * SectorSize
	var total int64
	var page [PageSize]byte
	for i := int64(0); i < l.size; i += PageSize {
		if _, err := l.dev.ReadAt(page[:], (start+i)%l.size); err != nil {
			return total, err
		}
		text := page[:0]
		for _, b := range page {
			if b != erased {
				text = append(text, b)
			}
		}
		n, err := w.Write(text)
		total += int64(n)
		if err != nil {
			return total, err
		}
	}
	return total, nil
}
