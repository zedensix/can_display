// The modules as they sit once assembled, for the assembly pictures only
// (never printed): the display and the encoder in the faceplate's frame,
// the boards and the converter in the tray's. Outlines, faces and
// connectors come from config.ts; the small parts (chips, the MCP2515's
// crystal and terminal) are only placed roughly, to show which face is
// which.

import type { ScadObject } from "scad-js";
import { cylinder, difference, hull, union } from "scad-js";
import type { Config } from "./config";
import { box, ENCODER_PINS, type Faceplate } from "./faceplate";
import type { Tray } from "./tray";

const PITCH = 2.54;

/** A row of square header pins along X (or Y), centered on (x, y), from z0 to z1. */
function pinRow(count: number, x: number, y: number, z0: number, z1: number, alongY = false): ScadObject {
  return union(
    ...Array.from({ length: count }, (_, i) => {
      const offset = (i - (count - 1) / 2) * PITCH;
      const [px, py] = alongY ? [x, y + offset] : [x + offset, y];
      return box(px - 0.32, px + 0.32, py - 0.32, py + 0.32, z0, z1);
    }),
  ).color("gold");
}

/** A header's plastic spacer under a row of pins. */
function spacer(count: number, x: number, y: number, z0: number, z1: number, alongY = false): ScadObject {
  const half = (count * PITCH) / 2;
  const [hx, hy] = alongY ? [PITCH / 2, half] : [half, PITCH / 2];
  return box(x - hx, x + hx, y - hy, y + hy, z0, z1).color("black");
}

/** The display in its pockets: the panel lit, the board, the solder joints at both ends, the header's pins out the back. */
function display(c: Config, faceplate: Faceplate): ScadObject {
  const { display: d, plate } = c;
  const boardFront = plate.frontSkin + d.glassThickness;
  const boardBack = boardFront + d.pcbThickness;
  const headerY = -d.pcbLength + 2;
  return union(
    box(
      -d.glassWidth / 2,
      d.glassWidth / 2,
      faceplate.glassTop - d.glassLength,
      faceplate.glassTop,
      plate.frontSkin,
      boardFront,
    ).color("black"),
    box(
      -d.activeWidth / 2,
      d.activeWidth / 2,
      faceplate.activeTop - d.activeLength,
      faceplate.activeTop,
      plate.frontSkin - 0.05,
      plate.frontSkin,
    ).color("orangered"),
    box(-d.pcbWidth / 2, d.pcbWidth / 2, -d.pcbLength, 0, boardFront, boardBack).color("royalblue"),
    ...[headerY, -2].map((y) =>
      box(-d.solderRowWidth / 2, d.solderRowWidth / 2, y - 1, y + 1, boardFront - d.solderHeight, boardFront).color(
        "goldenrod",
      ),
    ),
    spacer(8, 0, headerY, boardBack, boardBack + 2.5),
    pinRow(8, 0, headerY, boardFront - d.solderHeight, boardBack + 8.5),
  );
}

/**
 * The encoder: collar and shaft out the front, body in its recess, board
 * on its standoffs, its bent pins and plugs on the back of the board, or
 * in their channel under it on the knob side (encoder.pinsDepth above 0).
 */
function encoder(c: Config, faceplate: Faceplate): ScadObject {
  const e = c.encoder;
  const y = faceplate.shaftY;
  const face = e.panelThickness;
  const boardFace = faceplate.encoderBoardFace;
  const half = e.bodySize / 2;
  const back = boardFace + 1.6;
  const knobSide = e.pinsDepth > 0;
  const bend = knobSide ? boardFace - 3.5 : back + 3.5; // where the pins turn outward
  const [spacerFrom, spacerTo] = knobSide ? [boardFace - 2.5, boardFace] : [back, back + 2.5];
  // Through the board, its solder joints sticking out the other side.
  const [pinFrom, pinTo] = knobSide ? [bend - 0.32, back + 1] : [boardFace - 1, bend + 0.32];
  const pinsEnd = faceplate.encoderBoardRight + 4;
  const pinYs = Array.from({ length: ENCODER_PINS }, (_, i) => y + (i - (ENCODER_PINS - 1) / 2) * PITCH);
  return union(
    box(-half, half, y - half, y + half, face, boardFace).color("silver"),
    cylinder(e.bushingLength, e.bushingDiameter / 2, { center: false })
      .translate([0, y, face - e.bushingLength])
      .color("darkgray"),
    cylinder(15, 3, { center: false })
      .translate([0, y, face - e.bushingLength - 15])
      .color("lightgray"),
    box(
      faceplate.encoderBoardLeft,
      faceplate.encoderBoardRight,
      y - e.boardWidth / 2,
      y + e.boardWidth / 2,
      boardFace,
      back,
    ).color("midnightblue"),
    spacer(ENCODER_PINS, e.pinsFromShaft, y, spacerFrom, spacerTo, true),
    pinRow(ENCODER_PINS, e.pinsFromShaft, y, pinFrom, pinTo, true),
    // Bent outward along the board, past its end, a Dupont plug on each.
    union(
      ...pinYs.map((py) => box(e.pinsFromShaft - 0.32, pinsEnd, py - 0.32, py + 0.32, bend - 0.32, bend + 0.32)),
    ).color("gold"),
    union(
      ...pinYs.map((py) =>
        box(pinsEnd - 6, pinsEnd + 8, py - PITCH / 2 + 0.05, py + PITCH / 2 - 0.05, bend - PITCH / 2, bend + PITCH / 2),
      ),
    ).color("dimgray"),
  );
}

/** The Pico W, its components toward the tray (the USB socket between the cut standoffs), its pins pointing out. */
function pico(c: Config): ScadObject {
  const { tray } = c;
  const board = c.boards.pico;
  const [x, y] = board.center;
  const base = tray.thickness + tray.standoffHeight;
  const right = x + board.width / 2;
  const left = x - board.width / 2;
  const rowY = 8.89; // the pin rows, 17.78 apart
  return union(
    box(left, right, y - board.length / 2, y + board.length / 2, base, base + 1).color("green"),
    box(
      right + 1 - board.usbSocketDepth,
      right + 1,
      y - board.usbSocketWidth / 2,
      y + board.usbSocketWidth / 2,
      base - 2.6,
      base,
    ).color("silver"),
    box(x - 3.5, x + 3.5, y - 3.5, y + 3.5, base - 1, base).color("black"), // RP2040
    box(left + 4, left + 14, y - 5, y + 5, base - 1.6, base).color("silver"), // the wireless chip's shield
    ...[-rowY, rowY].flatMap((dy) => [
      spacer(20, x, y + dy, base + 1, base + 3.5),
      pinRow(20, x, y + dy, base - 1.5, base + 9.5),
    ]),
  );
}

/** The MCP2515 board, components out: the CAN terminal at the top, the 7-pin header at the bottom. */
function mcp2515(c: Config): ScadObject {
  const { tray } = c;
  const board = c.boards.mcp2515;
  const [x, y] = board.center;
  const base = tray.thickness + tray.standoffHeight;
  const face = base + 1.6;
  const top = y + board.length / 2;
  const headerY = y - board.length / 2 + 2;
  return union(
    box(x - board.width / 2, x + board.width / 2, y - board.length / 2, top, base, face).color("navy"),
    box(x - 5, x + 5, top - 7.5, top, face, face + 10).color("deepskyblue"), // terminal: H, L
    box(x - 12, x - 2, y - 3, y + 4.5, face, face + 1.5).color("black"), // MCP2515
    box(x + 6, x + 11, y - 1, y + 3, face, face + 1.5).color("black"), // TJA1050
    box(x - 12, x - 1, y - 9, y - 4.5, face, face + 3.5).color("silver"), // 8 MHz crystal
    spacer(7, x, headerY, face, face + 2.5),
    pinRow(7, x, headerY, base - 1.5, face + 8.5),
  );
}

/** The converter across the tray's extension, on its pad, a screw head in each ear's slot. */
function converter(c: Config, tray: Tray): ScadObject {
  const k = c.converter;
  const [, y] = tray.converterCenter;
  const z = c.tray.thickness + c.tray.converterPad;
  const ears = [-1, 1].map((side) => {
    const atBox = (side * k.length) / 2;
    const atEnd = side * (k.length / 2 + k.earLength);
    const slotX = atEnd - side * k.earSlotDepth;
    // A thin slice at each end, hulled into the tapered ear.
    const slice = (x: number, width: number) => box(x - 0.01, x + 0.01, y - width / 2, y + width / 2, z, z + 2);
    return difference(
      hull(slice(atBox, k.earWidthAtBox), slice(atEnd, k.earWidthAtEnd)),
      box(
        Math.min(slotX, atEnd + side),
        Math.max(slotX, atEnd + side),
        y - k.earSlotWidth / 2,
        y + k.earSlotWidth / 2,
        z - 1,
        z + 3,
      ),
    );
  });
  return union(
    box(-k.length / 2, k.length / 2, y - k.width / 2, y + k.width / 2, z, z + k.height).color("dimgray"),
    union(...ears).color("darkslategray"),
    ...tray.converterScrews.map(([sx, sy]) =>
      cylinder(2, 2.75, { center: false })
        .translate([sx, sy, z + 2])
        .color("lightgray"),
    ),
  );
}

/** Every module in place: the faceplate's in its frame, the tray's in the tray's (move them with it). */
export function mockups(c: Config, faceplate: Faceplate, tray: Tray): { faceplate: ScadObject; tray: ScadObject } {
  return {
    faceplate: union(display(c, faceplate), encoder(c, faceplate)),
    tray: union(pico(c), mcp2515(c), converter(c, tray)),
  };
}
