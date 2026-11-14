// Package settings holds what the settings page changes and how it's
// stored: a few bytes the firmware keeps in a flash sector of its own.
package settings

// Settings are the user's choices from the settings page.
type Settings struct {
	// Brightness is the backlight level, in percent.
	Brightness int
	// AutoConnect connects to the OBD dongle at power-on, and reconnects
	// by itself after a failure, instead of waiting for a click.
	AutoConnect bool
	// Flip turns the picture upside down, for a panel mounted that way.
	Flip bool
}

// Brightness bounds and step, in percent.
const (
	MinBrightness  = 10
	MaxBrightness  = 100
	BrightnessStep = 10
)

// Default is what a Pico that never saved settings uses.
var Default = Settings{Brightness: MaxBrightness}

// magic marks a flash sector holding settings, followed by a format
// version.
const (
	magic   = "GT86"
	version = 1
)

// Size is how many bytes Encode returns.
const Size = len(magic) + 3

const (
	flagAutoConnect = 1 << iota
	flagFlip
)

// Encode packs s for flash.
func (s Settings) Encode() []byte {
	var flags byte
	if s.AutoConnect {
		flags |= flagAutoConnect
	}
	if s.Flip {
		flags |= flagFlip
	}
	return append([]byte(magic), version, byte(s.Brightness), flags)
}

// Decode unpacks what Encode wrote. Anything else (erased flash, another
// format) gives Default.
func Decode(b []byte) Settings {
	if len(b) < Size || string(b[:len(magic)]) != magic || b[len(magic)] != version {
		return Default
	}
	brightness := int(b[len(magic)+1])
	if brightness < MinBrightness || brightness > MaxBrightness {
		return Default
	}
	flags := b[len(magic)+2]
	return Settings{
		Brightness:  brightness,
		AutoConnect: flags&flagAutoConnect != 0,
		Flip:        flags&flagFlip != 0,
	}
}
