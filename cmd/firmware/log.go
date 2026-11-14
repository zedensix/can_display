//go:build tinygo

package main

import (
	"machine"
	"strconv"
	"time"

	"zedensix/can_display/internal/flashlog"
)

// logSize is the flash kept for the log, at the very end of the 2MB so a
// bigger firmware doesn't land on it. Reflashing keeps it. 64KB is a
// few hundred boots' worth of connection logs.
const logSize = 64 * 1024

var (
	flashLog *flashlog.Log
	bootTime = time.Now()
)

func openLog() {
	log, err := flashlog.Open(flashRegion{start: machine.Flash.Size() - logSize}, logSize)
	if err != nil {
		println("log: flash unavailable:", err.Error())
		return
	}
	flashLog = log
}

// logLine prints line on the serial console and keeps it in the flash
// log, prefixed with the time since boot. Only for events, never for
// anything per frame or per OBD reply: every line is a flash write.
func logLine(line string) {
	line = "+" + strconv.FormatFloat(time.Since(bootTime).Seconds(), 'f', 1, 64) + "s " + line
	println(line)
	if flashLog == nil {
		return
	}
	if err := flashLog.Println(line); err != nil {
		println("log: write:", err.Error())
	}
}

// serveLog prints the flash log on the serial console when it receives
// 'l' (see the logs mise task). Called by the main loop.
func serveLog() {
	if flashLog == nil || machine.Serial.Buffered() == 0 {
		return
	}
	if b, err := machine.Serial.ReadByte(); err != nil || b != 'l' {
		return
	}
	println("--- flash log ---")
	if _, err := flashLog.WriteTo(machine.Serial); err != nil {
		println("log: read:", err.Error())
	}
	println("--- end of log ---")
}

// flashRegion is the log's place in flash as a flashlog.Device. start is
// a machine.Flash offset, counted from the end of the program.
type flashRegion struct {
	start int64
}

func (r flashRegion) ReadAt(p []byte, off int64) (int, error) {
	return machine.Flash.ReadAt(p, r.start+off)
}

func (r flashRegion) WriteAt(p []byte, off int64) (int, error) {
	return machine.Flash.WriteAt(p, r.start+off)
}

// EraseSector works around machine.Flash.EraseBlocks counting blocks from
// the end of the program, which isn't sector aligned: the chip erases the
// whole sector holding the address it's given, so this picks the block
// number that lands inside the wanted sector.
func (r flashRegion) EraseSector(off int64) error {
	block := (r.start + off + flashlog.SectorSize - 1) / flashlog.SectorSize
	return machine.Flash.EraseBlocks(block, 1)
}
