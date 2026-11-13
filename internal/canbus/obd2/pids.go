package obd2

// Standard SAE J1979 Mode 01 PIDs that any OBD-II compliant ECU should
// answer. These are the fallback for when there's no direct access to
// the car's broadcast frames (e.g. talking through an ELM327 dongle,
// which only relays request/response traffic -- see internal/elm327).
const (
	// PIDCoolantTemp is "Engine coolant temperature": A - 40 degC.
	PIDCoolantTemp byte = 0x05

	// PIDEngineRPM is "Engine speed": (256*A + B) / 4 RPM.
	PIDEngineRPM byte = 0x0C
)

// CoolantTempC decodes a PIDCoolantTemp response payload.
func CoolantTempC(payload []byte) (float32, bool) {
	if len(payload) < 1 {
		return 0, false
	}
	return float32(payload[0]) - 40, true
}

// EngineRPM decodes a PIDEngineRPM response payload.
func EngineRPM(payload []byte) (float32, bool) {
	if len(payload) < 2 {
		return 0, false
	}
	raw := uint16(payload[0])<<8 | uint16(payload[1])
	return float32(raw) / 4, true
}
