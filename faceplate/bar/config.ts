// Dimensions (mm) of the bar, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
// The bar (bar.ts): the modules in one long, thin box, laid along its
// length rather than stacked, the display in the middle: on one side the
// encoder, then room for the converter's micro-USB plug into the Pico's
// socket (the converter stays out, on the cable by the fuse box); on the
// other, the MCP2515. Its bottom is hollowed into an arc to sit on the
// steering column's shroud.
// The display, the Pico and the lid are built as shared/case.ts does, from
// enclosure below.
// The box's shared build (shared/case.ts): the display in landscape and
// the Pico W right behind it, on heat-staked pins, a lid on the walls.
// Seen from the back: x across, y up, the front at z = 0.
export const enclosure = {
  wall: 1.6, // around the display's board: 4 lines of a 0.4 mm nozzle
  cornerRadius: 3,
  // The Pico stands on tall standoffs, either way up: its debug connector
  // (about 5 mm), its USB socket and plug, and the wires fit on either side.
  picoComponents: 5.5, // from the Pico's board to the display board's back: the USB socket (2.6), the wires soldered on its pins' tips and bent over (a pin's tip and solder 1.5, a 26 AWG wire bent 2.5), the display board's own parts
  picoHeaderHeight: 2.5, // the header's plastic under the Pico's board, its pins cut flush with it
  picoStandoff: 6, // from the lid to the Pico's board: room for its debug connector (5), its USB plug and the wires
  picoPcbThickness: 1,
  picoUsbOverhang: 1.3, // how far its micro-USB socket stands past its board's end
  standoffDiameter: 3.4, // the Pico's 4 standoffs on the lid, between its header's 2 rows of plastic (4.4 ran 0.2 mm into them); each cut flat inside, boards.pico.usbSocketWidth and clearance between them, so the USB socket passes whichever way up the Pico goes
  // Heat-staked pins hold the boards, no screws: through each board's
  // holes (clearance smaller than the hole), then flattened into a head with
  // the soldering iron.
  stakeLength: 1.5, // how far each pin stands past its board, to melt into a head
  // The Pico's 2 pins by its USB socket stand either side of it, 9.6 mm
  // apart (holes 11.2 apart, pins 1.6 across) for a socket 8 mm wide: a
  // full head (about 3 mm) would touch it. Shorter, they melt into a head
  // about 2.3 mm across, 0.5 mm clear.
  stakeNearUsb: 0.6,
  lidThickness: 1.2, // the lid, lying on the walls' back edge all round
  // The lid's bumps click into slots through the top and bottom walls.
  tabWidth: 10, // each slot, along the wall
  tabLength: 8.5, // from the lid's face down to the slots
  hookLength: 1.2, // each bump's ramp, from its tip back to its catch
  usbSlotWidth: 12, // the opening over the Pico's micro-USB plug, for its overmold
  cableHole: 7, // the cable's slot through the lid: across
  cableSlotLength: 16, // and along: room for 2 cables, to the OBD end (6 wires) and to the encoder (4)
};

// Seen from the back: x along, y up, the front at z = 0.
export const bar = {
  frontWall: 1.6, // the front under the MCP2515, where nothing shows
  gap: 3, // from the display's room to the MCP2515
  endRoom: 8, // from the MCP2515 to the end wall: its terminal's wires, and the cable's slot over them
  encoderEnd: 1, // from the encoder's board and standoffs to the end wall
  // Between the display and the encoder, the converter's plug in the Pico's
  // socket, under an opening in the lid (enclosure.usbSlotWidth across) its cable
  // goes out through, where a finger pulls it out to plug the computer in.
  // It takes what's left once the display sits in the middle.
  plugRoomMin: 24, // the plug (about 18 with its strain relief) and its cable bending out
  mcpStandoff: 3, // the MCP2515 on 4 standoffs, its solder joints under it
  mcpStandoffDiameter: 6,
  mcpPcbThickness: 1.6,
  mcpTallest: 10, // its terminal block, the tallest part, above its board: it must clear the lid
  // The bottom, hollowed into an arc along the bar, its axis front to back
  // like the steering column's: laid on the shroud's top.
  // The lid: a tray as deep as lidExtra, sitting on the shell's walls, the
  // Pico on its back as deep further back; a skirt down inside the top and
  // bottom walls, a bump into each of the shell's slots. No tab: a skirt
  // that long bends along its length, a short tab across its layers, and
  // snaps (the tabs all broke).
  lidExtra: 15, // the wires' room: 19 mm deep wasn't enough
  skirtThickness: 1.2,
  bumpDepth: 0.6, // how far each bump reaches into the shell's slot
  // The lid's vents: slots across its back, quicker to print than a honeycomb.
  ventWidth: 3,
  ventPitch: 8,
  // The base (base.ts), stuck on the shroud with VHB tape, the bar clipped
  // into it as it is: a floor under the bar's arc, a hook up each end wall
  // and over the top, a lip across the front and the back. Pull a hook
  // outward to lift that end out.
  baseThickness: 2, // the floor, the hooks and the lips
  baseLip: 3, // how far each hook's lip reaches over the bar's top, past its rounded corner (enclosure.cornerRadius)
  // The lips across the front and the back, so the bar can't slide out
  // either way, rising this far up the bar from the floor.
  baseLipFront: 3, // under the display's window and its bevel (3.85 mm there)
  baseLipBack: 4, // under the lid's vent slots; past 1.5 mm, its inside slopes at 45 degrees: it prints last, in the air
  shroudSag: 5, // how much deeper the arc is at the ends than in the middle: a straight edge on the shroud showed 5 (test strips of 1 to 3 didn't reach)
};
