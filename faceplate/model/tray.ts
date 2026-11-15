// The tray: a flat plate with the faceplate's outline, screwed onto its 4
// posts, carrying on its back the Pico and the MCP2515 on standoffs and,
// between them, the converter, its ears cut off. The Pico sits components
// out: its 2 rows of header pins go through 2 slots, the Dupont plugs on
// them in the gap toward the faceplate, with the display's and the
// encoder's; a rib on the back stiffens each thin bridge the slots leave
// at the tray's edges. A rim runs along the edge on both faces, notched
// around the posts and for the Pico's micro-USB plug. The MCP2515's wires go around the tray's bottom edge.
//
// Its front face (toward the faceplate) is at z = 0, the standoffs stand
// up from its back; same X and Y as the faceplate. It prints front face
// down, standing on its front rim: the plate needs supports under it.

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
/** The converter's stops, at its ends, and the zip tie around it and through the tray. */
const CONVERTER_STOP = { thickness: 2, height: 5 };
const ZIP_TIE = { width: 4.5, thickness: 1.5 };
/**
 * A rib on the back over each bridge left at a Pico slot's end, between
 * the slot and the tray's edge: without them the tray snapped there, off
 * the bed or under the converter. past: how far it runs past the slot.
 */
const SLOT_RIB = { height: 6, past: 1 };

export class Tray {
  /** Each board's standoffs: center, outer radius, height, in the faceplate's X and Y. */
  readonly standoffs: { x: number; y: number; radius: number; height: number; pilotDiameter: number }[];
  /** The Pico's 2 pin rows' slots (x0, x1, y0, y1), wide enough for the Dupont plugs to come through. */
  readonly picoSlots: [number, number, number, number][];
  /** The ribs over the bridges at both ends of each slot (x0, x1, y0, y1). */
  readonly slotRibs: [number, number, number, number][];
  /** The converter's middle on the tray's back, and the zip tie's 2 holes beside it (x0, x1, y0, y1). */
  readonly converterY: number;
  readonly zipTieHoles: [number, number, number, number][];

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

    // From the slot's end, or the board's edge if further out, to the tray's edge.
    const ribFrom = Math.max(halfRow, pico.width / 2 + clearance);
    this.slotRibs = this.picoSlots.flatMap(([, , y0, y1]) =>
      [-1, 1].map((side): [number, number, number, number] => {
        const [x0, x1] = [side * ribFrom, side * faceplate.halfWidth].sort((a, b) => a - b);
        return [picoX + x0, picoX + x1, y0 - SLOT_RIB.past, y1 + SLOT_RIB.past];
      }),
    );
    if (faceplate.halfWidth - Math.abs(picoX) - ribFrom < 0.8) {
      throw new Error("boards.pico: its slots leave under 0.8 mm at the tray's edge, too thin to hold or to rib");
    }
    // The converter's micro-USB plug comes in at the Pico's right end, between the 2 ribs.
    const plugHalf = pico.usbSocketWidth / 2 + 1.5;
    if (this.slotRibs.some(([, , y0, y1]) => y1 > picoY - plugHalf && y0 < picoY + plugHalf)) {
      throw new Error("boards.pico.holesApartY: a slot's rib would block the micro-USB plug");
    }

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
    }

    const { converter } = c;
    this.converterY = -converter.fromTop;
    const converterHalfY = converter.width / 2 + clearance;
    this.zipTieHoles = [-1, 1].map((side) => {
      const y = this.converterY + side * (converterHalfY + ZIP_TIE.thickness / 2 + 0.5);
      return [-ZIP_TIE.width / 2, ZIP_TIE.width / 2, y - ZIP_TIE.thickness / 2, y + ZIP_TIE.thickness / 2];
    });
    if (converter.length / 2 + clearance + CONVERTER_STOP.thickness > faceplate.halfWidth - 0.5) {
      throw new Error("converter.length: too long for the tray, even with its ears cut off");
    }
    // Its footprint, the zip tie's holes included, clear of the boards and the tray's screws.
    const reach = converterHalfY + ZIP_TIE.thickness + 0.5;
    for (const [name, board] of Object.entries(boards)) {
      const [, centerY = 0] = board.center;
      if (Math.abs(centerY - this.converterY) < board.length / 2 + reach) {
        throw new Error(`converter.fromTop: the converter runs into boards.${name}`);
      }
    }
    if (faceplate.posts.some(([, y]) => Math.abs(y - this.converterY) < reach + tray.postDiameter / 2)) {
      throw new Error("converter.fromTop: the converter runs into a tray screw");
    }
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
    const { tray, rim, clearance, boards } = this.c;
    const pico = boards.pico;
    const [, picoY = 0] = pico.center;
    const usbHalf = pico.usbSocketWidth / 2 + 1.5;
    return difference(
      union(
        this.faceplate.outline(tray.thickness),
        this.faceplate.rim(rim.height + 0.01).translate([0, 0, -rim.height]),
        this.faceplate.rim(rim.height + 0.01).translate([0, 0, tray.thickness - 0.01]),
        this.converterStops,
        ...this.slotRibs.map(([x0, x1, y0, y1]) =>
          box(x0, x1, y0, y1, tray.thickness - 0.01, tray.thickness + SLOT_RIB.height),
        ),
        ...this.standoffs.map(({ x, y, radius, height }) =>
          cylinder(height + 0.01, radius, { center: false }).translate([x, y, tray.thickness - 0.01]),
        ),
      ),
      ...this.faceplate.posts.map(([x, y]) =>
        cylinder(tray.thickness + 2, tray.screwClearance / 2, { center: false }).translate([x, y, -1]),
      ),
      // The rims stop around each post (it sits in the notch) and its screw's head.
      ...this.faceplate.posts.flatMap(([x, y]) =>
        [-rim.height - 1, tray.thickness].map((z) =>
          cylinder(rim.height + 1, tray.postDiameter / 2 + clearance, { center: false }).translate([x, y, z]),
        ),
      ),
      // The back rim stops for the converter's micro-USB plug, at the Pico's right end.
      box(
        this.faceplate.halfWidth - rim.width - 1,
        this.faceplate.halfWidth + 1,
        picoY - usbHalf,
        picoY + usbHalf,
        tray.thickness,
        tray.thickness + rim.height + 1,
      ),
      ...this.zipTieHoles.map(([x0, x1, y0, y1]) => box(x0, x1, y0, y1, -99, 99)),
      // The Pico's pin rows and their plugs, through the tray, its rims and the standoffs' edges.
      ...this.picoSlots.map(([x0, x1, y0, y1]) => box(x0, x1, y0, y1, -99, 99)),
      // Blind pilots, so the front face stays closed under each board.
      ...this.standoffs.map(({ x, y, height, pilotDiameter }) =>
        cylinder(tray.thickness + height, pilotDiameter / 2, { center: false }).translate([x, y, 0.8]),
      ),
    );
  }

  /** A low stop past each end of the converter, so it can't slide across. */
  private get converterStops(): ScadObject {
    const { converter, clearance, tray } = this.c;
    const half = converter.length / 2 + clearance;
    const y0 = this.converterY - converter.width / 2;
    const y1 = this.converterY + converter.width / 2;
    return union(
      ...[-1, 1].map((side) => {
        const [x0, x1] = [side * half, side * (half + CONVERTER_STOP.thickness)].sort((a, b) => a - b);
        return box(x0, x1, y0, y1, tray.thickness - 0.01, tray.thickness + CONVERTER_STOP.height);
      }),
    );
  }
}
