package tests

import (
	"testing"

	"zedensix/can_display/internal/sensors"
)

func approxEqual(a, b, tolerance float32) bool {
	diff := a - b
	if diff < 0 {
		diff = -diff
	}
	return diff <= tolerance
}

func TestRawToVolts(t *testing.T) {
	cases := []struct {
		raw  uint16
		vref float32
		want float32
	}{
		{0, 3.3, 0},
		{65535, 3.3, 3.3},
		{32768, 3.3, 1.65},
	}
	for _, c := range cases {
		got := sensors.RawToVolts(c.raw, c.vref)
		if !approxEqual(got, c.want, 0.01) {
			t.Errorf("sensors.RawToVolts(%d, %v) = %v, want ~%v", c.raw, c.vref, got, c.want)
		}
	}
}

func TestDividerRoundTrip(t *testing.T) {
	d := sensors.Divider{R1: 10_000, R2: 22_000}

	sensorVolts := float32(4.5)
	measured := sensorVolts * d.Ratio()
	recovered := d.Undo(measured)

	if !approxEqual(recovered, sensorVolts, 0.001) {
		t.Errorf("Undo(sensorVolts * Ratio()) = %v, want %v", recovered, sensorVolts)
	}
}

func TestEthanolDividerStaysUnderRailVoltage(t *testing.T) {
	// The whole point of sensors.EthanolDivider: the sensor's worst-case output
	// must land safely under a 3.3V rail once divided.
	const maxSafeVolts = 3.3
	worstCase := sensors.EthanolSensorMaxVolts * sensors.EthanolDivider.Ratio()
	if worstCase >= maxSafeVolts {
		t.Errorf("divided sensor max = %vV, want comfortably under %vV", worstCase, maxSafeVolts)
	}
}
