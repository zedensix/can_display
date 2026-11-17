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
import { head } from "../head/config";
import { plateConfig } from "../plate/config";
import { pod } from "../pod/config";

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
  ...plateConfig,
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
  pod,
  head,
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
