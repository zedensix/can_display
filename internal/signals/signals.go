// Package signals holds the current state of every value the dashboard
// displays, independent of where that value came from (a real CAN frame,
// a replayed log, or the fake data generator used by the simulator).
package signals

import (
	"sync"
	"time"
)

// Reading is a single scalar value, timestamped so the UI can tell "real
// data" apart from "never seen" or "hasn't updated in a while" (bus
// unplugged, engine off, wrong CAN ID, sensor not wired up yet).
type Reading struct {
	Value     float32
	Valid     bool
	UpdatedAt time.Time
}

// Stale reports whether this reading is either unset or old enough that
// the UI shouldn't trust it anymore.
func (r Reading) Stale(now time.Time, maxAge time.Duration) bool {
	if !r.Valid {
		return true
	}
	return now.Sub(r.UpdatedAt) > maxAge
}

// State holds the latest known reading for every signal the dashboard
// cares about. It's written by one goroutine (the CAN decoder loop, or
// the simulator's generator) and read by the render loop, so all access
// goes through the exported methods rather than touching fields directly.
type State struct {
	mu sync.RWMutex

	coolantTempC   Reading
	oilTempC       Reading
	batteryVoltage Reading
	afr            Reading
	ethanolPercent Reading
	rpm            Reading

	status string
}

// Snapshot is a point-in-time copy of State, safe to read without a lock.
type Snapshot struct {
	CoolantTempC   Reading
	OilTempC       Reading
	BatteryVoltage Reading
	AFR            Reading
	EthanolPercent Reading
	RPM            Reading
	// Status is a short report from the data source while it isn't
	// delivering data (connecting, failed...). Empty once it runs.
	Status string
	At     time.Time
}

// Snapshot copies out the current readings.
func (s *State) Snapshot() Snapshot {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return Snapshot{
		CoolantTempC:   s.coolantTempC,
		OilTempC:       s.oilTempC,
		BatteryVoltage: s.batteryVoltage,
		AFR:            s.afr,
		EthanolPercent: s.ethanolPercent,
		RPM:            s.rpm,
		Status:         s.status,
		At:             time.Now(),
	}
}

func (s *State) SetCoolantTempC(v float32, at time.Time)   { s.set(&s.coolantTempC, v, at) }
func (s *State) SetOilTempC(v float32, at time.Time)       { s.set(&s.oilTempC, v, at) }
func (s *State) SetBatteryVoltage(v float32, at time.Time) { s.set(&s.batteryVoltage, v, at) }
func (s *State) SetAFR(v float32, at time.Time)            { s.set(&s.afr, v, at) }
func (s *State) SetEthanolPercent(v float32, at time.Time) { s.set(&s.ethanolPercent, v, at) }
func (s *State) SetRPM(v float32, at time.Time)            { s.set(&s.rpm, v, at) }

// SetStatus replaces Snapshot.Status. Lines are separated by "\n".
func (s *State) SetStatus(status string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.status = status
}

func (s *State) set(field *Reading, v float32, at time.Time) {
	s.mu.Lock()
	defer s.mu.Unlock()
	field.Value = v
	field.Valid = true
	field.UpdatedAt = at
}
