// --- Tunable dimensions (mm) ---
// The defaults come from sellers' listings of these modules, which
// disagree with each other: measure yours with calipers before printing.
// What you measured goes in measured.json, which overrides these: the
// wiring page's Dimensions tab (`bun run wiring`) writes it.
// The display stands in portrait (240 wide, 320 tall), the encoder below it.
// Positions are seen from the back of the dash, where you screw things on:
// x across (0 in the middle, + to the right), y up (0 at the display
// board's top edge).
import measured from "./measured.json";

const defaults = {
  display: {
    pcbWidth: 37.5, // the module's board, across (the short side)
    pcbLength: 58, // the module's board, top to bottom (the long side)
    pcbThickness: 1.6, // the board alone, without the glass
    holeInset: 2.5, // from the board's edges to the center of each of its 4 corner mounting holes
    glassWidth: 35.7, // the panel glued on the board (glass and backlight), across
    glassLength: 48, // the panel, top to bottom, without its ribbon
    glassThickness: 2.6, // the panel, glass and backlight, from the board's face
    glassFromPcbTop: 4.5, // from the board's top edge (away from the header) to the panel's top edge
    ribbonAllowance: 0.5, // extra room above the panel, where its ribbon folds around to the board: the end away from the soldered header
    activeWidth: 30.6, // the lit area (240 pixels), across, centered on the panel
    activeLength: 40.8, // the lit area (320 pixels), top to bottom
    activeFromGlassTop: 2.5, // from the panel's top edge to the lit area's top edge
    solderRowWidth: 20, // the pin header's solder joints on the board's face, across, centered, at its bottom end
    solderHeight: 1, // how far those joints stand off the board's face: the plate is relieved under them
  },
  screw: {
    pilotDiameter: 1.7, // hole an M2 self-tapping screw bites into: the display's and the encoder's boards, a board's holes under 3 mm
    m3PilotDiameter: 2.5, // hole an M3 self-tapping screw bites into: a board's holes of 3 mm or more
  },
  encoder: {
    bushingDiameter: 7, // the collar the shaft comes out of, threaded or not (M7x0.75 on a threaded EC11)
    bushingLength: 5, // the collar's length, from the encoder's face
    nutThickness: 0, // the nut clamping a threaded collar to the plate, washer included; 0 without thread: the board's screws alone hold it
    bodySize: 12.6, // the encoder's square metal body, its side facing the board's holes (they sit right against it)
    bodyHeight: 6.5, // from the board's face to the encoder's face, where the bushing starts
    panelThickness: 2, // the plate at the encoder, thin enough for the nut to grab the bushing
    belowDisplay: 16, // from the display board's bottom edge to the shaft's center
    // The board stands on 4 standoffs around the body. Its pin header
    // points to one side (left or right), inside the dash: its 2 mounting
    // holes then sit above or below the shaft, screwed with M2
    // self-tapping screws (screw.pilotDiameter); the 2 opposite standoffs
    // only hold the board level.
    holeSpacing: 13.75, // between the centers of the board's 2 mounting holes
    holesFromShaft: 7.9, // from the shaft's center to the line through the 2 holes
    standoffDiameter: 4.5, // the board's standoffs, cut flat where they meet the body
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
    standoffHeight: 5, // under each board: clears the solder joints, and the Pico's parts facing the tray (boards.pico.componentsHeight)
    standoffWall: 1.8, // around each standoff's pilot hole
    wireSlot: { width: 40, length: 7, center: [0, -51.25] }, // where the wires go through the tray, in the boards' frame below
  },
  // Each board on the tray's back, seen from the back of the dash, where
  // you screw it on. width runs along x, length along y; its 4 holes on a
  // rectangle centered on the board, holesApartX by holesApartY between
  // their centers. center: where the board's middle sits on the tray.
  boards: {
    pico: {
      width: 51,
      length: 21,
      holesApartX: 47, // Raspberry Pi's drawing
      holesApartY: 11.4,
      holeDiameter: 2.1, // the board's mounting holes: under 3 mm takes M2 screws (screw.pilotDiameter)
      center: [0, -12.5], // across the top, its USB socket out over the right edge
      // The micro-USB socket faces the tray, centered at the right end:
      // wider than the room between the 2 standoffs beside it, which are
      // cut flat to let it through.
      usbSocketWidth: 8,
      usbSocketDepth: 6, // from the board's right edge inward
      componentsHeight: 4.3, // the tallest part on the face toward the tray: the WH's 3-pin DEBUG connector
    },
    mcp2515: {
      width: 40, // blue MCP2515 + TJA1050 board
      length: 28,
      holesApartX: 35,
      holesApartY: 23,
      holeDiameter: 3, // the board's mounting holes: 3 mm or more takes M3 screws (screw.m3PilotDiameter)
      center: [0, -70],
    },
  },
  // The potted 12V-to-5V converter (its 5V comes out on a micro-USB plug
  // for the Pico). Not on the tray: with its ears it's 74 long, the tray
  // is narrower; it's zip-tied on the harness behind the dash. Here for
  // the wiring page's Dimensions tab.
  converter: {
    length: 46,
    width: 27.5,
    height: 14,
    earLength: 14, // one ear on each short end, centered across the width
    earWidthAtBox: 13,
    earWidthAtEnd: 10,
    earSlotWidth: 5, // a slot open at the ear's end, for the screw
    earSlotDepth: 7,
  },
  clearance: 0.3, // play around each pocket and hole, each side
};

export type Config = typeof defaults;

/** The number at a dotted path ("display.pcbWidth") in the config, or undefined. */
export function numberAt(config: Config, path: string): number | undefined {
  // biome-ignore lint/suspicious/noExplicitAny: walking the config by name
  const value = path.split(".").reduce<any>((node, part) => node?.[part], config);
  return typeof value === "number" ? value : undefined;
}

/**
 * The keys measured with calipers on the actual modules, or confirmed by a
 * test print, with their values (measured.json); every other dimension is
 * a seller's figure or a guess.
 */
export const checked: Record<string, number> = measured;

for (const [path, value] of Object.entries(checked)) {
  if (numberAt(defaults, path) === undefined) throw new Error(`measured.json: ${path} isn't a dimension in config.ts`);
  const parts = path.split(".");
  // biome-ignore lint/suspicious/noExplicitAny: walking the config by name
  const parent = parts.slice(0, -1).reduce<any>((node, part) => node[part], defaults);
  parent[parts.at(-1) as string] = value;
}

export const config: Config = defaults;
