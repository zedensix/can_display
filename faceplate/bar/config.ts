// Dimensions (mm) of the bar, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
// The bar (bar.ts): the modules in one long, thin box, laid along its
// length rather than stacked, the display in the middle: on one side the
// encoder, then room for the converter's micro-USB plug into the Pico's
// socket (the converter stays out, on the cable by the fuse box); on the
// other, the MCP2515. Its bottom is hollowed into an arc to sit on the
// steering column's shroud.
// The display, the Pico and the lid are built as the head's (head/config.ts).
// Seen from the back: x along, y up, the front at z = 0.
export const bar = {
  frontWall: 1.6, // the front under the MCP2515, where nothing shows
  gap: 3, // from the display's room to the MCP2515
  endRoom: 8, // from the MCP2515 to the end wall: its terminal's wires, and the cable's slot over them
  encoderEnd: 1, // from the encoder's board and standoffs to the end wall
  // Between the display and the encoder, the converter's plug in the Pico's
  // socket, under an opening in the lid (head.usbSlotWidth across) its cable
  // goes out through, where a finger pulls it out to plug the computer in.
  // It takes what's left once the display sits in the middle.
  plugRoomMin: 24, // the plug (about 18 with its strain relief) and its cable bending out
  mcpStandoff: 3, // the MCP2515 on 4 standoffs, its solder joints under it
  mcpStandoffDiameter: 6,
  mcpPcbThickness: 1.6,
  mcpTallest: 10, // its terminal block, the tallest part, above its board: it must clear the lid
  // The bottom, hollowed into an arc along the bar, its axis front to back
  // like the steering column's: laid on the shroud's top.
  // A lip under the lid, inside the walls all round, clear of the tabs: a
  // flat lid that long bends, and lets its hooks out.
  lipHeight: 2,
  lipWidth: 1.6,
  shroudSag: 5, // how much deeper the arc is at the ends than in the middle: a straight edge on the shroud showed 5, bar-arcs' 1 to 3 agreed (not enough)
  // bar-arcs, a test print to pick shroudSag: a strip as long as the bar
  // for each sag, as many notches on its top as its rank.
  gaugeSags: [1, 1.5, 2, 2.5, 3],
  gaugeHeight: 8,
  gaugeThickness: 2,
};
