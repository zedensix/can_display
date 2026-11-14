// --- Tunable dimensions (mm) ---
// The defaults come from sellers' listings of these modules, which
// disagree with each other: measure yours with calipers before printing.
// The display stands in portrait (240 wide, 320 tall), the encoder below it.
// Positions are seen from the back of the dash, where you screw things on:
// x across (0 in the middle, + to the right), y up (0 at the display
// board's top edge).
export const config = {
  display: {
    pcbWidth: 35.7, // the module's board, across (the short side)
    pcbLength: 60, // the module's board, top to bottom (the long side)
    pcbThickness: 1.6, // the board alone, without the glass
    holeInset: 2.5, // from the board's edges to the center of each of its 4 corner mounting holes
    glassWidth: 35.7, // the panel glued on the board (glass and backlight), across
    glassLength: 48, // the panel, top to bottom, without its ribbon
    glassThickness: 2.6, // the panel, glass and backlight, from the board's face
    glassFromPcbTop: 6, // from the board's top edge to the panel's top edge
    ribbonAllowance: 1.5, // extra room below the panel, where its ribbon folds around to the board
    activeWidth: 30.6, // the lit area (240 pixels), across, centered on the panel
    activeLength: 40.8, // the lit area (320 pixels), top to bottom
    activeFromGlassTop: 2.5, // from the panel's top edge to the lit area's top edge
  },
  screw: {
    pilotDiameter: 1.7, // hole an M2 self-tapping screw bites into: the display's and the encoder's boards
  },
  encoder: {
    bushingDiameter: 7, // the threaded bushing the shaft comes out of (M7x0.75 on an EC11)
    bushingLength: 5, // threaded length, from the encoder's face
    nutThickness: 2, // the nut that clamps the encoder to the plate, washer included
    bodySize: 12.6, // the encoder's square metal body, its side facing the board's holes (they sit right against it)
    bodyHeight: 6.5, // from the board's face to the encoder's face, where the bushing starts
    panelThickness: 2, // the plate at the encoder, thin enough for the nut to grab the bushing
    belowDisplay: 16, // from the display board's bottom edge to the shaft's center
    // The board's 2 mounting holes (KY-040 / HW-040: 13.75 apart, beside
    // the body), from the shaft's center: with its pin header pointing
    // down, they're on the right. Screwed with M2 self-tapping screws
    // (screw.pilotDiameter) into 2 standoffs.
    boardHoles: [
      [7.9, 6.875],
      [7.9, -6.875],
    ],
    standoffDiameter: 4.5, // the standoffs under the board's holes, cut flat where they meet the body
  },
  plate: {
    frontSkin: 1.2, // material in front of the display glass
    margin: 8, // plate around the display board and the encoder body: wide enough for tray.postDiameter beside the board
    cornerRadius: 3, // rounding of the plate's corners, seen from the front
  },
  window: {
    margin: 0.5, // window larger than the lit area on each side, so no pixel is hidden
    chamfer: 0.8, // 45 degree bevel on the window's front edge, at most plate.frontSkin
  },
  // The tray screws onto 4 posts at the faceplate's corners, behind the
  // display, and carries the boards on standoffs on its back.
  tray: {
    thickness: 2.5,
    gap: 20, // from the faceplate's back to the tray: room for the display's and the encoder's connectors (8 with soldered wires)
    postDiameter: 6.5, // the faceplate's posts
    screwPilot: 2.5, // hole an M3 self-tapping screw bites into, in each post
    screwClearance: 3.4, // the M3 screw's hole through the tray
    standoffHeight: 4, // under each board: clears the solder joints, and the Pico's USB socket facing the tray
    standoffWall: 1.8, // around each standoff's pilot hole
    wireSlot: { width: 40, length: 7, center: [0, -51.25] }, // where the wires go through the tray, in the boards' frame below
  },
  // Each board on the tray's back. center: where its middle sits. width
  // runs along x, length along y. holes: from the board's bottom-left
  // corner, as you look at it screwing it on.
  boards: {
    pico: {
      width: 51,
      length: 21,
      holes: [
        [2, 4.8],
        [49, 4.8],
        [2, 16.2],
        [49, 16.2],
      ], // Raspberry Pi's drawing: 47 x 11.4 apart
      pilotDiameter: 1.7, // M2 self-tapping
      center: [0, -12.5], // across the top, its USB socket out over the side
    },
    buck: {
      width: 43,
      length: 21,
      holes: [
        [4, 3],
        [39, 18],
      ], // LM2596 module, 2 holes on a diagonal: a guess, measure yours
      pilotDiameter: 2.5, // M3 self-tapping
      center: [0, -36.5],
    },
    mcp2515: {
      width: 40,
      length: 28,
      holes: [
        [2.5, 2.5],
        [37.5, 2.5],
        [2.5, 25.5],
        [37.5, 25.5],
      ], // blue MCP2515 + TJA1050 board: 35 x 23 apart
      pilotDiameter: 2.5, // M3 self-tapping
      center: [0, -70],
    },
  },
  clearance: 0.3, // play around each pocket and hole, each side
};

export type Config = typeof config;
