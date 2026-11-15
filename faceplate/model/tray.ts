// The tray: a flat plate with the faceplate's outline, screwed onto its 4
// posts, carrying the Pico and the MCP2515 on standoffs on its back, with
// a slot for the wires. Below the faceplate it widens into an extension
// where the 12V-to-5V converter lies across, on a pad, 2 M3 screws
// through the slots in its ears: with them it's wider than the faceplate.
//
// Its front face (toward the faceplate) is at z = 0, the standoffs stand
// up from its back; same X and Y as the faceplate. It prints front face
// down, no supports.

import type { ScadObject } from "scad-js";
import { cylinder, difference, square, union } from "scad-js";
import type { Config } from "./config";
import { box, type Faceplate } from "./faceplate";

type Board = Config["boards"][keyof Config["boards"]];

export class Tray {
  /** Each board's standoffs: center and outer radius, in the faceplate's X and Y. */
  readonly standoffs: { x: number; y: number; radius: number; pilotDiameter: number }[];
  /** The converter's center, just below the faceplate's outline, and its 2 screws, in the ears' slots. */
  readonly converterCenter: [number, number];
  readonly converterScrews: [number, number][];
  /** The extension's outline: its half width and its bottom edge. */
  readonly extensionHalfWidth: number;
  readonly bottom: number;

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { tray, boards } = c;
    this.standoffs = Object.values(boards).flatMap((board) =>
      Tray.holesOf(board).map(([x, y]) => ({
        x,
        y,
        radius: this.pilotOf(board) / 2 + tray.standoffWall,
        pilotDiameter: this.pilotOf(board),
      })),
    );

    const { converter } = c;
    const converterY = faceplate.bottom - 1 - converter.width / 2;
    this.converterCenter = [0, converterY];
    // Each screw at the bottom of its ear's slot.
    const screwX = converter.length / 2 + converter.earLength - converter.earSlotDepth + converter.earSlotWidth / 2;
    this.converterScrews = [
      [-screwX, converterY],
      [screwX, converterY],
    ];
    this.extensionHalfWidth = converter.length / 2 + converter.earLength + 1.5;
    this.bottom = converterY - converter.width / 2 - 1.5;
    if (converter.earSlotDepth < converter.earSlotWidth / 2 || converter.earSlotDepth > converter.earLength) {
      throw new Error("converter.earSlotDepth must be between half converter.earSlotWidth and converter.earLength");
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
      if ("componentsHeight" in board && board.componentsHeight + 0.3 > tray.standoffHeight) {
        throw new Error(`boards.${name}.componentsHeight: its parts would hit the tray, raise tray.standoffHeight`);
      }
      const keepout = this.keepoutOf(board);
      for (const [x, y] of Tray.holesOf(board)) {
        if (
          keepout &&
          x + this.pilotOf(board) / 2 > keepout[0] &&
          x - this.pilotOf(board) / 2 < keepout[2] &&
          y + this.pilotOf(board) / 2 > keepout[1] &&
          y - this.pilotOf(board) / 2 < keepout[3]
        ) {
          throw new Error(`boards.${name}.usbSocketWidth: the socket's room cuts into a screw hole`);
        }
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

  /** The faceplate's outline, widened below it into the converter's extension, through `thickness`. */
  outline(thickness: number): ScadObject {
    const r = this.c.plate.cornerRadius;
    const top = this.faceplate.bottom + r;
    return union(
      this.faceplate.outline(thickness),
      square([2 * this.extensionHalfWidth - 2 * r, top - this.bottom - 2 * r])
        .radius_offset(r)
        .linear_extrude(thickness)
        .translate([0, (top + this.bottom) / 2, 0]),
    );
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

  /**
   * The room a board needs between its standoffs (x0, y0, x1, y1), in the
   * faceplate's X and Y, clearance added: the Pico's USB socket, at its
   * right end, out over the edge by a millimeter.
   */
  keepoutOf(board: Board): [number, number, number, number] | null {
    if (!("usbSocketWidth" in board)) return null;
    const { clearance } = this.c;
    const [centerX = 0, centerY = 0] = board.center;
    const right = centerX + board.width / 2;
    return [
      right - board.usbSocketDepth - clearance,
      centerY - board.usbSocketWidth / 2 - clearance,
      right + 1 + clearance,
      centerY + board.usbSocketWidth / 2 + clearance,
    ];
  }

  get object(): ScadObject {
    const { tray } = this.c;
    const slot = tray.wireSlot;
    const [slotX, slotY] = slot.center;
    return difference(
      union(
        this.outline(tray.thickness),
        this.converterPad,
        ...this.standoffs.map(({ x, y, radius }) =>
          cylinder(tray.standoffHeight + 0.01, radius, { center: false }).translate([x, y, tray.thickness - 0.01]),
        ),
      ),
      ...this.faceplate.posts.map(([x, y]) =>
        cylinder(tray.thickness + 2, tray.screwClearance / 2, { center: false }).translate([x, y, -1]),
      ),
      box(slotX - slot.width / 2, slotX + slot.width / 2, slotY - slot.length / 2, slotY + slot.length / 2, -1, 99),
      // What a board needs room for between its standoffs (the Pico's USB socket).
      ...Object.values(this.c.boards).flatMap((board) => {
        const keepout = this.keepoutOf(board);
        return keepout ? [box(keepout[0], keepout[2], keepout[1], keepout[3], tray.thickness, 99)] : [];
      }),
      // Blind pilots, so the front face stays closed under each board and the converter.
      ...this.standoffs.map(({ x, y, pilotDiameter }) =>
        cylinder(tray.thickness + tray.standoffHeight, pilotDiameter / 2, { center: false }).translate([x, y, 0.8]),
      ),
      ...this.converterScrews.map(([x, y]) =>
        cylinder(tray.thickness + tray.converterPad, this.c.screw.m3PilotDiameter / 2, { center: false }).translate([
          x,
          y,
          0.8,
        ]),
      ),
    );
  }

  /** The pad the converter and its ears lie on, thick enough for its screws to bite. */
  private get converterPad(): ScadObject {
    const { tray, converter } = this.c;
    const [, y] = this.converterCenter;
    const halfLength = converter.length / 2 + converter.earLength + 0.5;
    const halfWidth = Math.max(converter.width, converter.earWidthAtBox) / 2 + 0.5;
    return box(
      -halfLength,
      halfLength,
      y - halfWidth,
      y + halfWidth,
      tray.thickness - 0.01,
      tray.thickness + tray.converterPad,
    );
  }
}
