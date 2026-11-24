// --- Tunable dimensions (mm) ---
// The defaults come from sellers' listings of these modules, which
// disagree with each other: measure yours with calipers before printing.
// What you measured goes in measured.json, which overrides these: the
// wiring page's Dimensions tab (`bun run wiring`) writes it.
// The display and the encoder are given in the front's frame (front.ts):
// the display in portrait (240 wide, 320 tall), the encoder below it,
// seen from the back: x across (0 in the middle, + to the right), y up (0
// at the display board's top edge). The bar turns them to landscape.

import { bar, enclosure } from "../bar/config";
import measured from "../data/measured.json";

const defaults = {
  display: {
    pcbWidth: 37.5, // the module's board, across (the short side)
    pcbLength: 58, // the module's board, top to bottom (the long side)
    pcbThickness: 1.6, // the board alone, without the glass
    // Its 4 corner mounting holes, on a rectangle centered on the board.
    holesApartX: 30.5, // between their centers, across
    holesApartY: 53, // between their centers, top to bottom
    holeDiameter: 3, // for the heat-staked pins through them
    glassWidth: 35.7, // the panel glued on the board (glass and backlight), across
    glassLength: 48, // the panel, top to bottom, without its ribbon
    glassThickness: 2.6, // the panel, glass and backlight, from the board's face
    glassFromPcbTop: 4.5, // from the board's top edge (the header's end, at the plate's outer edge) to the panel's top edge
    ribbonAllowance: 0.5, // extra room above the panel, where its ribbon folds around to the board
    activeWidth: 30.6, // the lit area (240 pixels), across, centered on the panel
    activeLength: 40.8, // the lit area (320 pixels), top to bottom
    activeFromGlassTop: 2.5, // from the panel's top edge to the lit area's top edge
    solderRowWidth: 20, // the solder joints on the board's face, across, centered, at each end (the pin header's above the panel, the pads below it)
    solderHeight: 1, // how far those joints stand off the board's face: the front is relieved under them
  },
  // The front the display and the encoder sit in (front.ts).
  front: {
    skin: 1.2, // material in front of the display glass
  },
  window: {
    margin: 0.5, // window larger than the lit area on each side, so no pixel is hidden
    chamfer: 0.8, // 45 degree bevel on the window's front edge, at most front.skin
  },
  encoder: {
    bushingDiameter: 7, // the collar the shaft comes out of, threaded or not (M7x0.75 on a threaded EC11)
    bushingLength: 5, // the collar's length, from the encoder's face
    nutThickness: 0, // the nut clamping a threaded collar to the front, washer included; 0 without thread: the board's pins alone hold it
    bodySize: 12.6, // the encoder's square metal body, its side facing the board's holes (they sit right against it)
    bodyHeight: 6.5, // from the board's face to the encoder's face, where the bushing starts
    panelThickness: 2, // the front under the encoder's body: the thinner, the further the collar and the shaft stick out (and a nut, if threaded, can grab the collar)
    belowDisplay: 16, // from the display board's bottom edge to the shaft's center
    // The board stands on 4 standoffs around the body, its long side
    // across, centered on the shaft, its pins to the right seen from the back: its 2 mounting
    // holes then sit above or below the shaft, on 2 heat-staked pins; the
    // 2 opposite standoffs only hold the board level.
    holeSpacing: 13.75, // between the centers of the board's 2 mounting holes
    holeDiameter: 3, // those holes (a guess: measure them), for the bar's heat-staked pins
    pcbThickness: 1.6,
    holesFromShaft: 7.9, // from the shaft's center to the line through the 2 holes
    holesTowardPins: 2, // from the shaft's center to the holes' midpoint, along the board toward the pins: a KY-040's holes aren't centered on the shaft (2 read off a photo, measure it)
    standoffDiameter: 4.5, // the board's standoffs, cut flat where they meet the body
    // Its 5 header pins, in a row across the board, on its back (for the
    // pictures: the wires are soldered on the encoder's legs).
    pinsFromShaft: 15, // from the shaft's center to the pin row, along the board
    boardLength: 26, // the board, from the end with the pins to the other one
    boardWidth: 19, // the board, across (the pin row's direction)
  },
  // Each board, seen from the back of the dash. width runs along x,
  // length along y; its 4 holes on a rectangle centered on the board,
  // holesApartX by holesApartY between their centers.
  boards: {
    pico: {
      width: 51,
      length: 21,
      holesApartX: 47, // Raspberry Pi's drawing
      holesApartY: 11.4,
      holeDiameter: 2.1,
      usbSocketWidth: 8, // the micro-USB socket, centered at the right end, on the outer face
      usbSocketDepth: 6, // from the board's right edge inward
    },
    mcp2515: {
      width: 40, // blue MCP2515 + TJA1050 board
      length: 28,
      holesApartX: 35,
      holesApartY: 23,
      holeDiameter: 3,
    },
  },
  enclosure,
  bar,
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
