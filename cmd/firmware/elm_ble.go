//go:build tinygo

package main

import (
	"device/arm"
	"errors"
	"strings"
	"time"

	"tinygo.org/x/bluetooth"

	"zedensix/can_display/internal/canbus"
	"zedensix/can_display/internal/canbus/gt86"
	"zedensix/can_display/internal/elm327"
	"zedensix/can_display/internal/signals"
)

// elmBLEAddress pins the dongle by MAC address ("AA:BB:CC:DD:EE:FF").
// Leave empty to take the first device that looks like an OBD dongle
// (see looksLikeELM). Every candidate is printed on the USB serial
// console, so plug the Pico into a computer once and read it from there
// if auto-detection picks the wrong thing.
const elmBLEAddress = ""

// elmNameHints are lowercase substrings of advertised names that mark a
// device as an OBD dongle. Vgate's usually advertise as "IOS-Vlink",
// "Android-Vlink" or "vLinker ...".
var elmNameHints = []string{"vlink", "vgate", "icar", "obd", "elm"}

// elmProfile is a GATT layout ELM327 BLE dongles use to expose their
// serial port: notifications carry the dongle's output, writes carry
// our commands. Which one a given dongle uses varies by model and
// firmware, hence the list. If yours isn't here, the serial console
// prints every service/characteristic it has: add a line.
type elmProfile struct {
	service, notify, write bluetooth.UUID
}

var elmProfiles = []elmProfile{
	{bluetooth.New16BitUUID(0xFFF0), bluetooth.New16BitUUID(0xFFF1), bluetooth.New16BitUUID(0xFFF2)},
	{bluetooth.New16BitUUID(0x18F0), bluetooth.New16BitUUID(0x2AF0), bluetooth.New16BitUUID(0x2AF1)},
	{bluetooth.New16BitUUID(0xFFE0), bluetooth.New16BitUUID(0xFFE1), bluetooth.New16BitUUID(0xFFE1)},
}

// batteryPollInterval: ATRV is answered by the dongle itself, without
// a round trip to the car, so it's cheap. Must stay under
// ui.DefaultStaleAfter.
const batteryPollInterval = time.Second

// runELM327BLE connects to the dongle and runs the OBD poll loop
// forever. Any failure (dongle unplugged, BLE dropped, car off) reboots
// the Pico, which is the simplest reliable reconnect: the whole thing
// starts over from the scan.
func runELM327BLE(reg *canbus.Registry, state *signals.State) {
	client, err := connectELMBLE()
	if err != nil {
		rebootAfter("BLE connect: " + err.Error())
	}
	println("elm327: resetting dongle")
	if err := client.Init(); err != nil {
		rebootAfter("ELM327 init: " + err.Error())
	}
	println("elm327: ready")

	bus := elm327.NewBus(client)
	go func() {
		// elm327.Bus.ReadFrame never fails, so this never returns.
		reg.Run(bus)
	}()
	go func() {
		for {
			if v, err := client.Voltage(); err == nil {
				state.SetBatteryVoltage(v, time.Now())
			}
			time.Sleep(batteryPollInterval)
		}
	}()
	if err := gt86.OBDPoller(bus).Run(); err != nil {
		rebootAfter("OBD poll: " + err.Error())
	}
}

// connectELMBLE scans until it finds the dongle, connects, finds its
// serial-port characteristics and returns a Client wired to them.
func connectELMBLE() (*elm327.Client, error) {
	adapter := bluetooth.DefaultAdapter
	if err := adapter.Enable(); err != nil {
		return nil, err
	}

	println("ble: scanning for the OBD dongle (phone's Bluetooth must be off or disconnected from it)")
	var found bluetooth.ScanResult
	var done bool
	err := adapter.Scan(func(a *bluetooth.Adapter, r bluetooth.ScanResult) {
		// Results already queued can still arrive after StopScan.
		if done || !looksLikeELM(r) {
			return
		}
		println("ble: candidate", r.Address.String(), r.LocalName(), r.RSSI)
		found, done = r, true
		a.StopScan()
	})
	if err != nil {
		return nil, err
	}

	println("ble: connecting to", found.Address.String())
	dev, err := adapter.Connect(found.Address, bluetooth.ConnectionParams{})
	if err != nil {
		return nil, err
	}

	notify, write, err := findELMCharacteristics(dev)
	if err != nil {
		return nil, err
	}

	client := elm327.NewClient(bleWriter{write})
	if err := notify.EnableNotifications(client.Receive); err != nil {
		return nil, err
	}
	return client, nil
}

func looksLikeELM(r bluetooth.ScanResult) bool {
	if elmBLEAddress != "" {
		return strings.EqualFold(r.Address.String(), elmBLEAddress)
	}
	for _, p := range elmProfiles {
		if r.HasServiceUUID(p.service) {
			return true
		}
	}
	name := strings.ToLower(r.LocalName())
	for _, h := range elmNameHints {
		if strings.Contains(name, h) {
			return true
		}
	}
	return false
}

func findELMCharacteristics(dev bluetooth.Device) (notify, write bluetooth.DeviceCharacteristic, err error) {
	services, err := dev.DiscoverServices(nil)
	if err != nil {
		return notify, write, err
	}
	for _, s := range services {
		println("ble: service", s.UUID().String())
		chars, err := s.DiscoverCharacteristics(nil)
		if err != nil {
			continue
		}
		for _, c := range chars {
			println("ble:   characteristic", c.UUID().String())
		}
		for _, p := range elmProfiles {
			if s.UUID() != p.service {
				continue
			}
			var haveN, haveW bool
			for _, c := range chars {
				if c.UUID() == p.notify {
					notify, haveN = c, true
				}
				if c.UUID() == p.write {
					write, haveW = c, true
				}
			}
			if haveN && haveW {
				return notify, write, nil
			}
		}
	}
	return notify, write, errNoELMService
}

var errNoELMService = errors.New("no known ELM327 serial service on this device, see elmProfiles")

// bleWriter sends commands as BLE "write without response" packets. The
// TinyGo HCI stack (Pico W) doesn't implement acknowledged writes. 20
// bytes is the payload of the default 23-byte ATT MTU; ELM commands are
// shorter than that anyway.
type bleWriter struct {
	c bluetooth.DeviceCharacteristic
}

func (w bleWriter) Write(p []byte) (int, error) {
	for sent := 0; sent < len(p); {
		end := sent + 20
		if end > len(p) {
			end = len(p)
		}
		if _, err := w.c.WriteWithoutResponse(p[sent:end]); err != nil {
			return sent, err
		}
		sent = end
	}
	return len(p), nil
}

// rebootAfter logs why and resets the RP2040 after a short pause (long
// enough to read the message on the serial console, short enough not to
// leave the dash dead for long).
func rebootAfter(reason string) {
	println("fatal:", reason, "-- rebooting")
	time.Sleep(3 * time.Second)
	arm.SystemReset()
}
