// The tray: a flat plate with the faceplate's outline, screwed onto its 4
// posts, carrying the Pico, the buck converter and the MCP2515 on
// standoffs on its back, with a slot for the wires.
//
// Its front face (toward the faceplate) is at z = 0, the standoffs stand
// up from its back; same X and Y as the faceplate. It prints front face
// down, no supports.

import type { ScadObject } from "scad-js";
import { cube, cylinder, difference, union } from "scad-js";
import type { Config } from "./config";
import { box, type Faceplate } from "./faceplate";

type Board = Config["boards"][keyof Config["boards"]];

export class Tray {
  /** Each board's standoffs: center and outer radius, in the faceplate's X and Y. */
  readonly standoffs: { x: number; y: number; radius: number; pilotDiameter: number }[];

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { tray, boards } = c;
    this.standoffs = Object.values(boards).flatMap((board) =>
      Tray.holesOf(board).map(([x, y]) => ({
        x,
        y,
        radius: board.pilotDiameter / 2 + tray.standoffWall,
        pilotDiameter: board.pilotDiameter,
      })),
    );

    const slot = tray.wireSlot;
    for (const [name, board] of Object.entries(boards)) {
      for (const [x, y] of Tray.holesOf(board)) {
        const radius = board.pilotDiameter / 2 + tray.standoffWall;
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

  /** A board's hole centers in the faceplate's X and Y: its holes are measured from the back, hence X mirrored. */
  static holesOf(board: Board): [number, number][] {
    const [centerX, centerY] = board.center;
    return board.holes.map(([holeX, holeY]): [number, number] => [
      centerX + board.width / 2 - (holeX ?? 0),
      centerY - board.length / 2 + (holeY ?? 0),
    ]);
  }

  get object(): ScadObject {
    const { tray } = this.c;
    const slot = tray.wireSlot;
    const [slotX, slotY] = slot.center;
    return difference(
      union(
        this.faceplate.outline(tray.thickness),
        ...this.standoffs.map(({ x, y, radius }) =>
          cylinder(tray.standoffHeight + 0.01, radius, { center: false }).translate([x, y, tray.thickness - 0.01]),
        ),
      ),
      ...this.faceplate.posts.map(([x, y]) =>
        cylinder(tray.thickness + 2, tray.screwClearance / 2, { center: false }).translate([x, y, -1]),
      ),
      box(slotX - slot.width / 2, slotX + slot.width / 2, slotY - slot.length / 2, slotY + slot.length / 2, -1, 99),
      // Blind pilots, so the front face stays closed under each board.
      ...this.standoffs.map(({ x, y, pilotDiameter }) =>
        cylinder(tray.thickness + tray.standoffHeight, pilotDiameter / 2, { center: false }).translate([x, y, 0.8]),
      ),
    );
  }

  /** The boards as flat slabs where they sit, for the assembly picture only. */
  get boardMockups(): ScadObject {
    const { tray, boards } = this.c;
    const colors: Record<keyof Config["boards"], string> = { pico: "green", buck: "royalblue", mcp2515: "navy" };
    return union(
      ...(Object.entries(boards) as [keyof Config["boards"], Board][]).map(([name, board]) =>
        cube([board.width, board.length, 1.6], { center: false })
          .translate([
            board.center[0] - board.width / 2,
            board.center[1] - board.length / 2,
            tray.thickness + tray.standoffHeight,
          ])
          .color(colors[name]),
      ),
    );
  }
}
