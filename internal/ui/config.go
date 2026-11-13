package ui

import "time"

// This file is the one place to tune what counts as "normal," "warning,"
// or "danger" for each gauge -- the numbers, not the layout. If a
// threshold needs adjusting for your car/engine/sensor, this is what to
// edit; internal/ui/dashboard.go just wires these into tiles and
// shouldn't need to change alongside them.
//
// See Range's doc comment (tile.go) for what each field means: Min/Max
// are the gauge's bar endpoints, WarnLow/WarnHigh/DangerLow/DangerHigh
// are the thresholds a value is compared against to pick a color.

// DefaultStaleAfter is how long a signal can go unrefreshed before the
// dashboard shows "--" instead of a frozen last value -- long enough to
// ride out normal bus jitter, short enough to notice a disconnected
// sensor or a car that's been switched off within a couple of seconds.
const DefaultStaleAfter = 2 * time.Second

// RPMRange thresholds. FA20 redline is ~7000-7600 RPM; WarnHigh/DangerHigh give a
// shift-light-style ramp into the rev limiter. No meaningful "too low"
// danger while running, so the low thresholds are set below the
// gauge's own range to never trigger.
var RPMRange = Range{Min: 0, Max: 8000, WarnLow: -1, WarnHigh: 7000, DangerLow: -1, DangerHigh: 7400}

// CoolantRange thresholds. Normal operating range for the FA20 is roughly
// 85-105°C; danger thresholds sit outside that with a small buffer so
// a brief blip (e.g. sitting in traffic before the fan kicks in) isn't
// immediately alarming.
var CoolantRange = Range{Min: 40, Max: 130, WarnLow: 60, WarnHigh: 105, DangerLow: 50, DangerHigh: 115}

// OilTempRange thresholds. Broader safe band than coolant since oil temp swings
// more with driving style (track use runs hotter than commuting) and
// takes longer to stabilize.
var OilTempRange = Range{Min: 40, Max: 150, WarnLow: 60, WarnHigh: 110, DangerLow: 50, DangerHigh: 120}

// BatteryRange thresholds. A healthy 12V system idles around 12.6-12.8V
// (engine off) or ~14-14.5V (alternator charging). Below ~11.5V or
// above ~15.5V both indicate a real electrical problem, not normal
// variation.
var BatteryRange = Range{Min: 10, Max: 16, WarnLow: 12, WarnHigh: 15, DangerLow: 11.5, DangerHigh: 15.5}

// AFRRange thresholds. Centered on stoichiometric (14.7) with warning bands
// opening up toward the rich/lean extremes a naturally-aspirated
// engine might actually see.
var AFRRange = Range{Min: 10, Max: 18, WarnLow: 12, WarnHigh: 16, DangerLow: 11, DangerHigh: 17}

// EthanolRange thresholds. No meaningful "danger" zone for ethanol content by
// itself (it's informational, used to judge fueling/timing, not a
// safety cutoff) -- thresholds sit outside [0, 100] so they never
// trigger.
var EthanolRange = Range{Min: 0, Max: 100, WarnLow: -1, WarnHigh: 101, DangerLow: -1, DangerHigh: 101}
