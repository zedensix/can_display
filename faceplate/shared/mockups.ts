// The modules as they sit once assembled, for the assembly pictures only
// (never printed): the display and the encoder in the front's frame
// (front.ts). Outlines, faces and
// connectors come from config.ts; the small parts (chips, the MCP2515's
// crystal and terminal) are only placed roughly, to show which face is
// which.

import type { ScadObject } from "scad-js";
import { cylinder, union } from "scad-js";
import type { Front } from "../shared/front";
import type { Config } from "./config";
import { box, DUPONT_LENGTH, ENCODER_PINS, HEADER_PINS, HEADER_PLASTIC, PITCH } from "./geometry";

/** A row of square header pins along X (or Y), centered on (x, y), from z0 to z1. */
export function pinRow(count: number, x: number, y: number, z0: number, z1: number, alongY = false): ScadObject {
  return union(
    ...Array.from({ length: count }, (_, i) => {
      const offset = (i - (count - 1) / 2) * PITCH;
      const [px, py] = alongY ? [x, y + offset] : [x + offset, y];
      return box(px - 0.32, px + 0.32, py - 0.32, py + 0.32, z0, z1);
    }),
  ).color("gold");
}

/** A header's plastic spacer under a row of pins. */
export function spacer(count: number, x: number, y: number, z0: number, z1: number, alongY = false): ScadObject {
  const half = (count * PITCH) / 2;
  const [hx, hy] = alongY ? [PITCH / 2, half] : [half, PITCH / 2];
  return box(x - hx, x + hx, y - hy, y + hy, z0, z1).color("black");
}

/** A row of Dupont plugs along X, centered on (x, y), from z0 to z1. */
export function plugRow(count: number, x: number, y: number, z0: number, z1: number): ScadObject {
  return union(
    ...Array.from({ length: count }, (_, i) => {
      const px = x + (i - (count - 1) / 2) * PITCH;
      return box(px - PITCH / 2 + 0.05, px + PITCH / 2 - 0.05, y - PITCH / 2 + 0.05, y + PITCH / 2 - 0.05, z0, z1);
    }),
  ).color("dimgray");
}

/** The display in its pockets: the panel lit, the board, the solder joints at both ends, the header's pins out the back. */
export function display(c: Config, front: Front): ScadObject {
  const { display: d, front: f } = c;
  const boardFront = f.skin + d.glassThickness;
  const boardBack = boardFront + d.pcbThickness;
  // The header at the board's top end, away from the encoder.
  const headerY = -2;
  return union(
    box(-d.glassWidth / 2, d.glassWidth / 2, front.glassTop - d.glassLength, front.glassTop, f.skin, boardFront).color(
      "black",
    ),
    box(
      -d.activeWidth / 2,
      d.activeWidth / 2,
      front.activeTop - d.activeLength,
      front.activeTop,
      f.skin - 0.05,
      f.skin,
    ).color("orangered"),
    box(-d.pcbWidth / 2, d.pcbWidth / 2, -d.pcbLength, 0, boardFront, boardBack).color("royalblue"),
    ...[headerY, -d.pcbLength + 2].map((y) =>
      box(-d.solderRowWidth / 2, d.solderRowWidth / 2, y - 1, y + 1, boardFront - d.solderHeight, boardFront).color(
        "goldenrod",
      ),
    ),
    spacer(8, 0, headerY, boardBack, boardBack + HEADER_PLASTIC),
    pinRow(8, 0, headerY, boardFront - d.solderHeight, boardBack + HEADER_PLASTIC + HEADER_PINS),
    plugRow(8, 0, headerY, boardBack + HEADER_PLASTIC, boardBack + HEADER_PLASTIC + DUPONT_LENGTH),
  );
}

/**
 * The encoder: collar and shaft out the front, body in its recess, board
 * on its standoffs, its bent pins and plugs on the back of the board.
 */
export function encoder(c: Config, front: Front): ScadObject {
  const e = c.encoder;
  const y = front.shaftY;
  const face = e.panelThickness;
  const boardFace = front.encoderBoardFace;
  const half = e.bodySize / 2;
  const back = boardFace + 1.6;
  const bend = back + 3.5; // where the pins turn outward
  const [spacerFrom, spacerTo] = [back, back + 2.5];
  // Through the board, its solder joints sticking out the other side.
  const [pinFrom, pinTo] = [boardFace - 1, bend + 0.32];
  const pinsEnd = front.encoderBoardRight + 4;
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
      front.encoderBoardLeft,
      front.encoderBoardRight,
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
