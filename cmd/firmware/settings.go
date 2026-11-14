//go:build tinygo

package main

import (
	"machine"

	"tinygo.org/x/drivers"
	"tinygo.org/x/drivers/st7789"

	"zedensix/can_display/internal/flashlog"
	"zedensix/can_display/internal/settings"
)

// settingsRegion is the flash sector holding the settings, right below
// the log, so reflashing keeps them too.
func settingsRegion() flashRegion {
	return flashRegion{start: machine.Flash.Size() - logSize - flashlog.SectorSize}
}

func loadSettings() settings.Settings {
	b := make([]byte, settings.Size)
	if _, err := settingsRegion().ReadAt(b, 0); err != nil {
		logLine("settings: read: " + err.Error())
		return settings.Default
	}
	return settings.Decode(b)
}

// saveSettings rewrites the settings sector. Called when leaving the
// settings page with something changed, so the flash barely wears.
func saveSettings(cfg settings.Settings) {
	region := settingsRegion()
	if err := region.EraseSector(0); err != nil {
		logLine("settings: erase: " + err.Error())
		return
	}
	if _, err := region.WriteAt(cfg.Encode(), 0); err != nil {
		logLine("settings: write: " + err.Error())
		return
	}
	logLine("settings: saved")
}

// setupBacklight drives the panel's BL pin with PWM, for the brightness
// setting, and returns what sets it, in percent. It must run after the
// st7789 driver, which configures that pin as a plain output.
func setupBacklight() func(percent int) {
	pwm := machine.PWM3                                                   // GP22 (displayBL) is PWM slice 3, channel A
	if err := pwm.Configure(machine.PWMConfig{Period: 1e6}); err != nil { // 1kHz, in ns
		logLine("backlight: " + err.Error())
		return func(int) {}
	}
	channel, err := pwm.Channel(displayBL)
	if err != nil {
		logLine("backlight: " + err.Error())
		return func(int) {}
	}
	return func(percent int) {
		pwm.Set(channel, pwm.Top()*uint32(percent)/100)
	}
}

// applySettings makes the panel follow cfg.
func applySettings(panel *st7789.Device, setBrightness func(int), cfg settings.Settings) {
	setBrightness(cfg.Brightness)
	rotation := drivers.Rotation(drivers.Rotation0)
	if cfg.Flip {
		rotation = drivers.Rotation180
	}
	if err := panel.SetRotation(rotation); err != nil {
		logLine("display: rotation: " + err.Error())
	}
}
