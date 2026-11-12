// Package sim fabricates plausible-looking signal data so the dashboard
// UI can be developed and demoed without a car.
package sim

import (
	"math"
	"math/rand"
	"time"

	"zedensix/can_display/internal/signals"
)

// Generator writes fake readings straight into a signals.State on every
// Tick, standing in for what a real canbus.Registry of decoders would
// otherwise do. It is not a CAN bus emulator -- it never produces
// frames, only final signal values.
type Generator struct {
	start time.Time
	rng   *rand.Rand

	// EnableAFR/EnableEthanol/EnableBattery let you preview what the
	// dashboard looks like for signals that don't have a *confirmed*
	// real data source yet (see internal/canbus/gt86 and
	// internal/sensors for what's actually wired up vs. still
	// speculative/missing). Off by default so the simulator's default
	// view matches what the firmware can reliably show today.
	EnableAFR, EnableEthanol, EnableBattery bool
}

// NewGenerator returns a Generator whose clock starts now.
func NewGenerator() *Generator {
	return &Generator{start: time.Now(), rng: rand.New(rand.NewSource(1))}
}

// Tick writes one fresh set of fake readings into state, timestamped at
// now.
func (g *Generator) Tick(state *signals.State, now time.Time) {
	t := now.Sub(g.start).Seconds()

	// Coolant/oil temp: ramp up from a cold start over ~90s, then hover
	// with small noise, like a warming engine settling at temperature.
	warmup := clamp(float32(t/90), 0, 1)
	coolant := 20 + warmup*70 + noise(g.rng, 1.5)
	oil := 20 + warmup*95 + noise(g.rng, 2)
	state.SetCoolantTempC(coolant, now)
	state.SetOilTempC(oil, now)

	// RPM: idles around 850 with periodic revs up toward redline, so
	// the RPM tile's warning/danger color bands are visible in the
	// simulator too, not just a flat idle number. Always on, like
	// coolant/oil temp -- RPM has a real decoder (internal/canbus/gt86),
	// it's not speculative like the Enable*-gated signals below.
	revFraction := clamp(float32(math.Sin(t/6)), 0, 1)
	rpm := 850 + revFraction*6700 + noise(g.rng, 50)
	state.SetRPM(rpm, now)

	if g.EnableBattery {
		vibration := float32(math.Sin(t / 5))
		state.SetBatteryVoltage(14.2+vibration*0.3+noise(g.rng, 0.05), now)
	}
	if g.EnableAFR {
		swing := float32(math.Sin(t / 2))
		state.SetAFR(14.7+swing*1.5+noise(g.rng, 0.2), now)
	}
	if g.EnableEthanol {
		state.SetEthanolPercent(10+noise(g.rng, 1), now)
	}
}

func noise(rng *rand.Rand, amplitude float32) float32 {
	return (rng.Float32()*2 - 1) * amplitude
}

func clamp(v, min, max float32) float32 {
	switch {
	case v < min:
		return min
	case v > max:
		return max
	default:
		return v
	}
}
