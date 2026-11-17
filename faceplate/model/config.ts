// --- Tunable dimensions (mm) ---
// The defaults come from sellers' listings of these modules, which
// disagree with each other: measure yours with calipers before printing.
// What you measured goes in measured.json, which overrides these: the
// wiring page's Dimensions tab (`bun run wiring`) writes it.
// The display stands in portrait (240 wide, 320 tall), the encoder below it.
// Positions are seen from the back of the dash, where you screw things on:
// x across (0 in the middle, + to the right), y up (0 at the display
// board's top edge).
import measured from "../data/measured.json";

const defaults = {
  display: {
    pcbWidth: 37.5, // the module's board, across (the short side)
    pcbLength: 58, // the module's board, top to bottom (the long side)
    pcbThickness: 1.6, // the board alone, without the glass
    holeInset: 2.5, // from the board's edges to the center of each of its 4 corner mounting holes
    glassWidth: 35.7, // the panel glued on the board (glass and backlight), across
    glassLength: 48, // the panel, top to bottom, without its ribbon
    glassThickness: 2.6, // the panel, glass and backlight, from the board's face
    glassFromPcbTop: 4.5, // from the board's top edge (the header's end, at the plate's outer edge) to the panel's top edge
    ribbonAllowance: 0.5, // extra room above the panel, where its ribbon folds around to the board
    activeWidth: 30.6, // the lit area (240 pixels), across, centered on the panel
    activeLength: 40.8, // the lit area (320 pixels), top to bottom
    activeFromGlassTop: 2.5, // from the panel's top edge to the lit area's top edge
    solderRowWidth: 20, // the solder joints on the board's face, across, centered, at each end (the pin header's above the panel, the pads below it)
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
    panelThickness: 2, // the plate under the encoder's body: the thinner, the further the collar and the shaft stick out (and a nut, if threaded, can grab the collar)
    belowDisplay: 16, // from the display board's bottom edge to the shaft's center
    // The board stands on 4 standoffs around the body, its long side
    // across, centered on the shaft, its pins to the right seen from the back: its 2 mounting
    // holes then sit above or below the shaft, screwed with M2
    // self-tapping screws (screw.pilotDiameter); the 2 opposite standoffs
    // only hold the board level.
    holeSpacing: 13.75, // between the centers of the board's 2 mounting holes
    holesFromShaft: 7.9, // from the shaft's center to the line through the 2 holes
    holesTowardPins: 2, // from the shaft's center to the holes' midpoint, along the board toward the pins: a KY-040's holes aren't centered on the shaft (2 read off a photo, measure it)
    standoffDiameter: 4.5, // the board's standoffs, cut flat where they meet the body
    // Its 5 header pins, in a row across the board, bent outward along it
    // past its end, the wires' plugs on them. Soldered on the back of the
    // board they point into the room behind the plate: nothing under the
    // board. Soldered on the knob side they'd hold the board off the plate,
    // so a channel runs under them out to the plate's edge.
    pinsDepth: 0, // room the pins and their plugs take under the board, on the knob side; 0: the header is on the back
    pinsFromShaft: 15, // from the shaft's center to the pin row, along the board
    boardLength: 26, // the board, from the end with the pins to the other one
    boardWidth: 19, // the board, across (the pin row's direction)
  },
  plate: {
    frontSkin: 1.2, // material in front of the display glass
    margin: 8, // plate around the display board and the encoder body: wide enough for tray.postDiameter beside the board
    cornerRadius: 3, // rounding of the plate's corners, seen from the front
  },
  // A lip along the edge, on the faceplate's back and the tray's: on their fronts, they'd print over supports.
  rim: {
    height: 2,
    width: 2,
  },
  window: {
    margin: 0.5, // window larger than the lit area on each side, so no pixel is hidden
    chamfer: 0.8, // 45 degree bevel on the window's front edge, at most plate.frontSkin
  },
  // The tray screws onto 4 posts at the faceplate's corners, behind the
  // display, and carries the boards on standoffs and the converter on its
  // back. Between the two: the display's and the encoder's plugs, and the
  // Pico's through the tray.
  tray: {
    thickness: 2.5,
    gap: 20, // from the faceplate's back to the tray: the Dupont plugs
    postDiameter: 6.5, // the faceplate's posts
    screwPilot: 2.5, // hole an M3 self-tapping screw bites into, in each post
    screwClearance: 3.4, // the M3 screw's hole through the tray
    standoffWall: 1.8, // around each standoff's pilot hole
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
      // Components out, its header toward the tray: its 2 rows of pins go
      // through 2 slots in the tray, the Dupont plugs on them in the gap.
      standoffHeight: 3, // the header's plastic (2.5 under the board) just clears the tray
      usbSocketWidth: 8, // the micro-USB socket, centered at the right end, on the outer face
      usbSocketDepth: 6, // from the board's right edge inward
    },
    mcp2515: {
      width: 40, // blue MCP2515 + TJA1050 board
      length: 28,
      holesApartX: 35,
      holesApartY: 23,
      holeDiameter: 3, // the board's mounting holes: 3 mm or more takes M3 screws (screw.m3PilotDiameter)
      center: [0, -70],
      standoffHeight: 5, // clears the solder joints under the board
    },
  },
  // The potted 12V-to-5V converter (its 5V comes out on a micro-USB plug
  // for the Pico). Its ears cut off, it lies across the tray's back,
  // between the Pico and the MCP2515, between 2 stops, a zip tie around
  // it through the tray. The ears are here as bought, for the drawing.
  converter: {
    fromTop: 39.5, // from the display board's top edge down to the converter's middle, on the tray
    length: 46,
    width: 27.5,
    height: 14,
    earLength: 14, // one ear on each short end, centered across the width
    earWidthAtBox: 13,
    earWidthAtEnd: 10,
    earSlotWidth: 5, // a slot open at the ear's end, for the screw
    earSlotDepth: 7,
  },
  // A temporary mount on an air vent's horizontal slats, for testing on
  // the road (ventmount.ts): a clamp on one slat, from a bar across the
  // tray's back held by the 2 top tray screws (M3x14 instead of M3x10), and
  // 2 feet on the bottom posts (M3x30) against what's under the vent.
  // Guesses: measure the vent.
  vent: {
    standoff: 20, // from the tray's back to the vent's face: past the converter (14)
    slatThickness: 2, // the slat the clamp takes, top to bottom
    slatDepth: 12, // that slat, front to back, up to the vertical vanes behind it: the clamp's legs stop 1 mm short
    slatGap: 8, // between two slats: each leg goes in one, above and below the slat
    legThickness: 3, // each leg, top to bottom
    legWidth: 12, // each leg, across
    wheelDiameter: 14, // the thumbwheel on the clamp's screw, between the tray and the vent
    feetLength: 20, // from the tray's back to what the dash's bottom rests on, under the vent (the radio's face)
  },
  // The pod: the whole dash in the driver's round air vent, in place of a
  // 52 mm gauge (pod.ts). Its front takes the outside of the gauge's
  // printed adapter (faceplate/resources/gauge_vent.stl): its 73 mm lip on
  // the driver's side, the display in landscape and the encoder under it,
  // the 83 mm body into the vent's OEM ring, the 2 ears at the back.
  // Behind, a can goes into the vent's duct, a sled in it carrying the
  // Pico, the MCP2515 and the converter. Seen from the back like the rest:
  // x across, y up, the front face at z = 0, z into the vent.
  pod: {
    bodyDiameter: 83, // the adapter's, which the vent's OEM ring holds
    lipDiameter: 73, // its front, standing out of the vent
    lipLength: 2.2, // then a 45 degree step out to bodyDiameter (the adapter's is steeper: this one prints front down)
    depth: 24.5, // from the front face to the back, where the ears are
    earWidth: 3, // the 2 ears at the back edge, left and right: across
    earOut: 1.5, // how far they stand out of the body
    earThickness: 2.5, // front to back
    wall: 2, // the body's wall around the room behind the face
    displayY: 2, // the display board's middle, above the pod's center
    shaftY: -24, // the encoder's shaft, below the pod's center
    // The can: into the vent's duct (15 cm and more free behind the
    // adapter), onto a spigot on the pod's back, 2 countersunk M3 screws
    // through its front band into the spigot.
    canDiameter: 52,
    canWall: 1.6,
    canBandLength: 8, // its front end, thicker for the screws' countersinks
    canBandWall: 2.5,
    capThickness: 2.5, // its back end, the cable's hole in it
    cableHole: 9,
    spigotLength: 4,
    spigotWall: 3,
    screwHead: 6, // the countersunk M3's head
    // The sled: a plate across the can's middle, sliding in between 2
    // pairs of ribs, held between the spigot and the can's back. On top
    // the Pico (USB toward the front) then the MCP2515 (terminal toward
    // the back), under it the converter, zip-tied.
    plateThickness: 3,
    ribDepth: 1.4, // the ribs, from the can's wall inward
    ribWidth: 1.2,
    zipSlotWidth: 3, // the zip ties' slots through the plate
    zipSlotLength: 5,
    notchDepth: 5, // notches in the plate's edges for the wires between its 2 sides
    notchLength: 8,
    gap: 4, // along the sled, between the boards
    // The vents, a honeycomb like the adapter's grille (podvents.ts).
    ventCell: 1.8, // each cell, center to corner
    ventWall: 0.8, // between two cells: 2 lines of a 0.4 mm nozzle
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
