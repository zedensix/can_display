// Dimensions (mm) of the head, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
// The head (head.ts): the display in landscape and the Pico W right
// behind it, the smallest box they go in, for up high (the meter hood's
// top); the MCP2515 and the converter at the OBD port, the encoder on its
// own, on a cable. Seen from the back: x across, y up, the front at z = 0.
export const head = {
  wall: 1.6, // around the display's board: 4 lines of a 0.4 mm nozzle
  cornerRadius: 3,
  displayBackGap: 1.5, // from the display board's back to the Pico's header plastic: the board's components
  picoHeaderHeight: 2.5, // the header's plastic under the Pico's board, toward the display, its pins cut flush with it; 0 without a header
  picoPcbThickness: 1,
  picoStandoff: 3.5, // from the Pico's board to the lid: its components (the USB socket, 2.6) and the wires soldered on its pins' tips
  picoUsbOverhang: 1.3, // how far its micro-USB socket stands past its board's end
  standoffDiameter: 4.4, // the Pico's 4 standoffs under the lid, M2 x 5 self-tapping screws
  lidThickness: 1.2,
  bossWidth: 5, // the lid's 2 screw bosses, on the top and bottom walls
  bossDepth: 4, // how far they stand out of the wall
  screwDepth: 4.5, // their pilot holes: M2 x 5 self-tapping screws through the lid
  screwClearance: 2.4, // the M2 screws' holes through the lid
  usbSlotWidth: 12, // the notch by the Pico's micro-USB socket, for the plug's overmold
  usbSlotHeight: 8,
  cableHole: 6, // through the lid, under the Pico: the wires to the MCP2515, the converter and the encoder
  tilt: 20, // degrees the screen leans back, a wedge under it, toward the eyes above it (guessed: set it from the hood's slope)
  ventCell: 1.8, // the lid's honeycomb, as the pod's
  ventWall: 0.8,
};
