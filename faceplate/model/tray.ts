// The tray: a flat plate with the faceplate's outline, screwed onto its 4
// posts, carrying the Pico and the MCP2515 on standoffs on its back, with
// a slot for the wires. The Pico sits components out: its 2 rows of
// header pins go through 2 slots, the Dupont plugs on them in the gap
// toward the faceplate, where the converter lies too.
//
// Its front face (toward the faceplate) is at z = 0, the standoffs stand
// up from its back; same X and Y as the faceplate. It prints front face
// down, no supports.

import type { ScadObject } from "scad-js";
import { cylinder, difference, union } from "scad-js";
import type { Config } from "./config";
import { box, type Faceplate, PITCH } from "./faceplate";

type Board = Config["boards"][keyof Config["boards"]];

/** The Pico's header (Raspberry Pi's drawing): 2 rows of 20 pins, 17.78 apart, centered on the board. */
export const PICO_PINS = 20;
export const PICO_ROWS_APART = 17.78;
/** A pin header's plastic, and its pins past it; a Dupont plug's length. */
export const HEADER_PLASTIC = 2.5;
export const HEADER_PINS = 6;
export const DUPONT_LENGTH = 14;

export class Tray {
  /** Each board's standoffs: center, outer radius, height, in the faceplate's X and Y. */
  readonly standoffs: { x: number; y: number; radius: number; height: number; pilotDiameter: number }[];
  /** The Pico's 2 pin rows' slots (x0, x1, y0, y1), wide enough for the Dupont plugs to come through. */
  readonly picoSlots: [number, number, number, number][];

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { tray, boards, clearance } = c;
    this.standoffs = Object.values(boards).flatMap((board) =>
      Tray.holesOf(board).map(([x, y]) => ({
        x,
        y,
        radius: this.pilotOf(board) / 2 + tray.standoffWall,
        height: board.standoffHeight,
        pilotDiameter: this.pilotOf(board),
      })),
    );
    const pico = boards.pico;
    const [picoX, picoY] = pico.center;
    const halfRow = (PICO_PINS * PITCH) / 2 + clearance;
    const halfSlot = PITCH / 2 + clearance;
    this.picoSlots = [-1, 1].map((side) => {
      const rowY = picoY + (side * PICO_ROWS_APART) / 2;
      return [picoX - halfRow, picoX + halfRow, rowY - halfSlot, rowY + halfSlot];
    });

    if (pico.standoffHeight < HEADER_PLASTIC + 0.3) {
      throw new Error(`boards.pico.standoffHeight: its header's plastic (${HEADER_PLASTIC} mm) would sit on the tray`);
    }
    // The plugs reach from the header's plastic, through the tray, into the gap.
    const plugsInGap = DUPONT_LENGTH - (pico.standoffHeight - HEADER_PLASTIC) - tray.thickness;
    if (plugsInGap > tray.gap - 2) {
      throw new Error("tray.gap: the Pico's Dupont plugs would touch the faceplate, raise it");
    }
    for (const [x0, x1, y0, y1] of this.picoSlots) {
      for (const [x, y] of Tray.holesOf(pico)) {
        const r = this.pilotOf(pico) / 2 + 0.6;
        if (x + r > x0 && x - r < x1 && y + r > y0 && y - r < y1) {
          throw new Error("boards.pico.holesApartY: a pin row's slot cuts into a screw hole");
        }
      }
      // The converter lies in the gap too: not under the Pico's plugs.
      const { converter } = c;
      if (
        y0 < -converter.fromTop + converter.width / 2 + clearance &&
        y1 > -converter.fromTop - converter.width / 2 - clearance &&
        x0 < converter.length / 2 &&
        x1 > -converter.length / 2
      ) {
        throw new Error("converter.fromTop: the converter lies under the Pico's plugs, make it larger");
      }
    }

    const slot = tray.wireSlot;
    for (const [name, board] of Object.entries(boards)) {
      const [centerX = 0, centerY = 0] = board.center;
      if (
        Math.abs(centerX) + board.width / 2 > faceplate.halfWidth + 1e-6 ||
        centerY + board.length / 2 > faceplate.top + 1e-6 ||
        centerY - board.length / 2 < faceplate.bottom - 1e-6
      ) {
        throw new Error(`boards.${name}: the board hangs off the tray, check its width and length or move its center`);
      }
      for (const [x, y] of Tray.holesOf(board)) {
        const radius = this.pilotOf(board) / 2 + tray.standoffWall;
        if (
          Math.abs(x) + radius > faceplate.halfWidth + 1e-6 ||
          y + radius > faceplate.top + 1e-6 ||
          y - radius < faceplate.bottom - 1e-6
        ) {
          throw new Error(`boards.${name}: a standoff falls off the tray, move its center`);
        }
        if (faceplate.posts.some(([px, py]) => Math.hypot(px - x, py - y) < radius + tray.postDiameter / 2)) {
          throw new Error(`boards.${name}: a standoff sits on one of the tray's screws, move its center`);
        }
        if (
          Math.abs(x - slot.center[0]) < slot.width / 2 + radius &&
          Math.abs(y - slot.center[1]) < slot.length / 2 + radius
        ) {
          throw new Error(`boards.${name}: a standoff runs into tray.wireSlot`);
        }
      }
    }
  }

  /** The pilot hole a board's screws bite into: M3 for holes of 3 mm or more, else M2. */
  pilotOf(board: Board): number {
    return board.holeDiameter >= 3 ? this.c.screw.m3PilotDiameter : this.c.screw.pilotDiameter;
  }

  /** A board's 4 hole centers in the faceplate's X and Y, both seen from the back. */
  static holesOf(board: Board): [number, number][] {
    const [centerX = 0, centerY = 0] = board.center;
    return [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [
        centerX + (sx * board.holesApartX) / 2,
        centerY + (sy * board.holesApartY) / 2,
      ]),
    );
  }

  get object(): ScadObject {
    const { tray } = this.c;
    const slot = tray.wireSlot;
    const [slotX, slotY] = slot.center;
    return difference(
      union(
        this.faceplate.outline(tray.thickness),
        ...this.standoffs.map(({ x, y, radius, height }) =>
          cylinder(height + 0.01, radius, { center: false }).translate([x, y, tray.thickness - 0.01]),
        ),
      ),
      ...this.faceplate.posts.map(([x, y]) =>
        cylinder(tray.thickness + 2, tray.screwClearance / 2, { center: false }).translate([x, y, -1]),
      ),
      box(slotX - slot.width / 2, slotX + slot.width / 2, slotY - slot.length / 2, slotY + slot.length / 2, -1, 99),
      // The Pico's pin rows and their plugs, through the tray and the standoffs' edges.
      ...this.picoSlots.map(([x0, x1, y0, y1]) => box(x0, x1, y0, y1, -1, 99)),
      // Blind pilots, so the front face stays closed under each board.
      ...this.standoffs.map(({ x, y, height, pilotDiameter }) =>
        cylinder(tray.thickness + height, pilotDiameter / 2, { center: false }).translate([x, y, 0.8]),
      ),
    );
  }
}
