// Dimensions (mm) of the head, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
// The head (head.ts): the display in landscape and the Pico W right
// behind it, the smallest box they go in, for up high (the meter hood's
// top); the MCP2515 and the converter at the OBD port, the encoder on its
// own, on a cable. Seen from the back: x across, y up, the front at z = 0.
export const head = {
  wall: 1.6, // around the display's board: 4 lines of a 0.4 mm nozzle
  cornerRadius: 3,
  // The Pico lies components toward the display, its header's plastic on
  // the lid: under the lid, standoffs beside its USB socket would block the plug.
  picoComponents: 5.5, // from the Pico's board to the display board's back: the USB socket (2.6), the wires soldered on its pins' tips and bent over (a pin's tip and solder 1.5, a 26 AWG wire bent 2.5), the display board's own parts
  picoHeaderHeight: 2.5, // the header's plastic under the Pico's board, on the lid, its pins cut flush with it; its standoffs as tall
  picoPcbThickness: 1,
  picoUsbOverhang: 1.3, // how far its micro-USB socket stands past its board's end
  standoffDiameter: 4.4, // the Pico's 4 standoffs on the lid, on its header's side
  // Heat-staked pins hold the boards, no screws: through each board's
  // holes (clearance smaller than the hole), then flattened into a head with
  // the soldering iron.
  stakeLength: 1.5, // how far each pin stands past its board, to melt into a head
  // The Pico's 2 pins by its USB socket stand either side of it, 9.6 mm
  // apart for a socket 8 mm wide: a full head (about 3 mm) would touch it.
  // Shorter, they melt into a head about 2.3 mm across, 0.5 mm clear.
  stakeNearUsb: 0.6,
  lidThickness: 1.2, // the lid, lying on the walls' back edge all round
  // 2 snap tabs hang from the lid along the top and bottom walls, a hook at
  // each tip clicking into a slot through the wall; a small screwdriver
  // through the slot pushes the hook back in to take the lid off.
  tabWidth: 8,
  tabThickness: 1, // thin enough to bend: PLA takes about 3% strain, the hook's bend here about 2.4%
  tabLength: 7, // from the lid down, ending above the display's board
  hookDepth: 0.5, // how far the hook stands out of the tab, into the slot
  hookLength: 1.2, // the hook's ramp, from the tip back to its catch
  usbSlotWidth: 12, // the notch by the Pico's micro-USB socket, for the plug's overmold
  usbSlotHeight: 8,
  cableHole: 7, // the cable's slot through the lid, under the Pico: across
  cableSlotLength: 16, // and along: room for 2 cables, to the OBD end (6 wires) and to the encoder (4)
  tilt: 20, // degrees the screen leans back, a wedge under it, toward the eyes above it (guessed: set it from the hood's slope)
  ventCell: 1.8, // the lid's honeycomb, as the pod's
  ventWall: 0.8,
};
