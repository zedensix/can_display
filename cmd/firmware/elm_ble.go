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
	client, err := connectELMBLE(state)
	if err != nil {
		rebootAfter(state, "BLE connect", err)
	}
	println("elm327: resetting dongle")
	state.SetStatus(statusTitle + "init ELM327...")
	if err := client.Init(); err != nil {
		rebootAfter(state, "ELM327 init", err)
	}
	println("elm327: ready")
	state.SetStatus("")

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
		rebootAfter(state, "OBD poll", err)
	}
}

// connectELMBLE scans until it finds the dongle, connects, finds its
// serial-port characteristics and returns a Client wired to them.
func connectELMBLE(state *signals.State) (*elm327.Client, error) {
	state.SetStatus(statusTitle + "starting Bluetooth...")
	adapter := bluetooth.DefaultAdapter
	if err := adapter.Enable(); err != nil {
		return nil, err
	}

	println("ble: scanning for the OBD dongle (phone's Bluetooth must be off or disconnected from it)")
	state.SetStatus(statusTitle + "scanning...")
	var found bluetooth.ScanResult
	var done bool
	err := adapter.Scan(func(a *bluetooth.Adapter, r bluetooth.ScanResult) {
		// Results already queued can still arrive after StopScan.
		if done {
			return
		}
		if !looksLikeELM(r) {
			state.SetStatus(statusTitle + "scanning...\nlast ignored:\n" + deviceName(r))
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
	state.SetStatus(statusTitle + "connecting to\n" + deviceName(found))
	dev, err := adapter.Connect(found.Address, bluetooth.ConnectionParams{})
	if err != nil {
		return nil, err
	}
	dongle = &dev

	state.SetStatus(statusTitle + "reading services of\n" + deviceName(found))
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

// dongle is the open BLE connection, closed before rebooting: a dongle
// left on a connection that vanished can stop answering for a while.
var dongle *bluetooth.Device

// statusTitle heads every status screen of this source.
const statusTitle = "OBD DONGLE\n"

func deviceName(r bluetooth.ScanResult) string {
	if r.LocalName() == "" {
		return r.Address.String()
	}
	return r.LocalName() + "\n" + r.Address.String()
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

// rebootAfter shows why on the panel and the serial console, then resets
// the RP2040 after a pause long enough to read it.
func rebootAfter(state *signals.State, step string, err error) {
	println("fatal:", step+":", err.Error(), "-- rebooting")
	state.SetStatus(statusTitle + step + " failed:\n" + err.Error() + "\nrebooting...")
	if dongle != nil {
		if err := dongle.Disconnect(); err != nil {
			println("ble: disconnect:", err.Error())
		}
	}
	time.Sleep(5 * time.Second)
	arm.SystemReset()
}
