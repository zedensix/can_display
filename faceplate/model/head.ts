// The head: the display in landscape with the Pico W right behind it, in
// the smallest box they go in, for up high (the meter hood's top). The
// rest stays out of sight on a cable: the MCP2515 and the converter at
// the OBD port, the encoder wherever the hand falls. The display's SPI
// (62.5 MHz) won't go down a long cable, the MCP2515's (1 MHz) and the
// encoder will: so the Pico sits behind the display.
//
// - The shell: the front with the display's window and pockets (the
//   faceplate's, turned to landscape), walls just around its board, a room
//   behind it for the Pico, a notch in the end by its USB socket for the
//   plug, 2 bosses for the lid's screws, and a wedge under it so the screen
//   leans back toward the driver's eyes, its face for 3M VHB tape.
// - The lid closes the back, the Pico screwed under it on 4 standoffs,
//   components toward the lid (the wires soldered on its pins' tips), its
//   header's plastic toward the display, the pins cut flush with it. Vents
//   around them, and the cable's hole under the Pico.
//
// Seen from the back like the faceplate: x across, y up, the front at
// z = 0, the display board centered. Both print with no supports: the
// shell front down (the window's bevel, the bosses' 45 degree
// undersides), the lid outside down.

import type { ScadObject } from "scad-js";
import { cylinder, difference, hull, intersection, square, union } from "scad-js";
import type { Config } from "./config";
import type { Faceplate } from "./faceplate";
import { box } from "./geometry";
import { display } from "./mockups";
import { cellHoles, honeycomb, type Rect } from "./podvents";
import { HEADER_PLASTIC, PICO_ROWS_APART } from "./tray";

export class Head {
  /** The room inside the walls: the display board and its clearance. */
  readonly halfX: number;
  readonly halfY: number;
  /** The front's thickness: the display board's back. */
  readonly plate: number;
  /** The Pico's board, its side toward the display, and the lid's inner face. */
  readonly picoFace: number;
  readonly lidFace: number;
  readonly depth: number;
  /** The Pico's board along x, its USB socket at the left end (seen from the back). */
  readonly picoX: [number, number];
  readonly picoHoles: [number, number][];
  /** The lid's screws, in the 2 bosses. */
  readonly screws: [number, number][];
  readonly cable: [number, number];

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { display: d, head: h, boards, clearance } = c;
    this.halfX = d.pcbLength / 2 + clearance;
    this.halfY = d.pcbWidth / 2 + clearance;
    this.plate = faceplate.thickness;
    this.picoFace = this.plate + h.displayBackGap + h.picoHeaderHeight;
    this.lidFace = this.picoFace + h.picoPcbThickness + h.picoStandoff;
    this.depth = this.lidFace + h.lidThickness;
    const x0 = -this.halfX + h.picoUsbOverhang + clearance;
    this.picoX = [x0, x0 + boards.pico.width];
    const cx = x0 + boards.pico.width / 2;
    this.picoHoles = [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [
        cx + (sx * boards.pico.holesApartX) / 2,
        (sy * boards.pico.holesApartY) / 2,
      ]),
    );
    this.screws = [-1, 1].map((sy): [number, number] => [0, sy * (this.halfY - h.bossDepth / 2)]);
    // Under the Pico, toward the hood, clear of the bottom boss.
    this.cable = [this.halfX / 2, -(boards.pico.length / 2 + this.halfY) / 2];

    if (this.halfY - boards.pico.length / 2 < h.cableHole + 1) {
      throw new Error("head: no room for the cable's hole under the Pico: check head.cableHole");
    }
    if (this.halfY - h.bossDepth < boards.pico.length / 2) {
      throw new Error("head: the lid's bosses reach the Pico: check head.bossDepth");
    }
    if (this.lidFace - h.screwDepth - h.bossDepth < this.plate) {
      throw new Error("head: the bosses' undersides reach the display's board: check head.screwDepth and bossDepth");
    }
  }

  /** The faceplate's display cutouts or mockup, turned to landscape, the board centered. */
  private displayed(o: ScadObject): ScadObject {
    return o.rotate([0, 0, -90]).translate([this.c.display.pcbLength / 2, 0, 0]);
  }

  /** A rounded rectangle, halfX by halfY, extruded from z0 to z1. */
  private rounded(halfX: number, halfY: number, r: number, z0: number, z1: number): ScadObject {
    return square([2 * halfX - 2 * r, 2 * halfY - 2 * r])
      .radius_offset(r)
      .linear_extrude(z1 - z0)
      .translate([0, 0, z0]);
  }

  /** The wedge under the bottom: its face, flat on the hood, leans the screen back by head.tilt. */
  private get foot(): ScadObject {
    const { head: h } = this.c;
    const x = this.halfX + h.wall - h.cornerRadius;
    const bottom = -this.halfY - h.wall;
    const drop = this.depth * Math.tan((h.tilt * Math.PI) / 180);
    return hull(
      box(-x, x, bottom - drop, bottom + 0.01, 0, 0.01),
      box(-x, x, bottom - 0.01, bottom + 0.01, this.depth - 0.01, this.depth),
    );
  }

  /** The lid's 2 bosses, on the top and bottom walls, their undersides at 45 degrees. */
  private get bosses(): ScadObject {
    const { head: h } = this.c;
    const w = h.bossWidth / 2;
    const bottom = this.lidFace - h.screwDepth;
    return union(
      ...[-1, 1].map((sy) => {
        const wall = sy * this.halfY;
        const inner = sy * (this.halfY - h.bossDepth);
        const [y0, y1] = sy > 0 ? [inner, wall + 0.01] : [wall - 0.01, inner];
        return hull(
          box(-w, w, y0, y1, bottom, this.lidFace),
          box(-w, w, wall - 0.01, wall + 0.01, bottom - h.bossDepth, bottom),
        );
      }),
    );
  }

  /** The shell, as built: front at z = 0, open at the back. */
  get shell(): ScadObject {
    const { head: h, screw } = this.c;
    const f = this.faceplate;
    const socketZ = this.picoFace + h.picoPcbThickness + 1.3;
    return difference(
      union(
        difference(
          union(this.rounded(this.halfX + h.wall, this.halfY + h.wall, h.cornerRadius, 0, this.depth), this.foot),
          box(-this.halfX, this.halfX, -this.halfY, this.halfY, this.plate, this.depth + 1),
        ),
        this.bosses,
      ),
      this.displayed(union(f.displayWindow, f.displayPockets, f.displayScrewHoles)),
      // The USB plug's notch, open at the back.
      box(
        -this.halfX - h.wall - 1,
        -this.halfX + 0.01,
        -h.usbSlotWidth / 2,
        h.usbSlotWidth / 2,
        socketZ - h.usbSlotHeight / 2,
        this.depth + 1,
      ),
      ...this.screws.map(([x, y]) =>
        cylinder(h.screwDepth + 1, screw.pilotDiameter / 2, { center: false }).translate([
          x,
          y,
          this.lidFace - h.screwDepth,
        ]),
      ),
    );
  }

  /** The lid, as built: from lidFace to the back, the Pico's standoffs toward the front. */
  get lid(): ScadObject {
    const { head: h, screw, clearance } = this.c;
    const lx = this.halfX - clearance;
    const ly = this.halfY - clearance;
    const standoffR = h.standoffDiameter / 2;
    const keepOut: Rect[] = [
      ...this.picoHoles.map(([x, y]): Rect => [x - standoffR, x + standoffR, y - standoffR, y + standoffR]),
      ...this.screws.map(([x, y]): Rect => [x - h.bossWidth / 2, x + h.bossWidth / 2, y - 2.5, y + 2.5]),
      [
        this.cable[0] - h.cableHole / 2,
        this.cable[0] + h.cableHole / 2,
        this.cable[1] - h.cableHole / 2,
        this.cable[1] + h.cableHole / 2,
      ],
    ];
    const r = h.ventCell;
    const cells = honeycomb(r, h.ventWall, 0, Math.hypot(lx, ly), keepOut).filter(
      ([x, y]) => Math.abs(x) + r + 1.5 <= lx && Math.abs(y) + r + 1.5 <= ly,
    );
    return difference(
      union(
        box(-lx, lx, -ly, ly, this.lidFace, this.depth),
        ...this.picoHoles.map(([x, y]) =>
          cylinder(h.picoStandoff + 0.01, standoffR, { center: false }).translate([
            x,
            y,
            this.lidFace - h.picoStandoff,
          ]),
        ),
      ),
      // Blind: the outside stays clean.
      ...this.picoHoles.map(([x, y]) =>
        cylinder(h.picoStandoff + 1, screw.pilotDiameter / 2, { center: false }).translate([
          x,
          y,
          this.lidFace - h.picoStandoff - 0.5,
        ]),
      ),
      ...this.screws.map(([x, y]) =>
        cylinder(h.lidThickness + 2, h.screwClearance / 2, { center: false }).translate([x, y, this.lidFace - 1]),
      ),
      cylinder(h.lidThickness + 2, h.cableHole / 2, { center: false }).translate([...this.cable, this.lidFace - 1]),
      cellHoles(cells, r, this.lidFace - 1, this.depth + 1),
    );
  }

  /** Each part to print, as it lies on the bed. */
  get print(): Record<string, ScadObject> {
    return {
      "head-shell": this.shell,
      "head-lid": this.lid.rotate([180, 0, 0]).translate([0, 0, this.depth]),
    };
  }

  /** Everything in place: for the pictures, never printed. */
  get assembly(): ScadObject {
    const { c } = this;
    const { boards, head: h } = c;
    const [x0, x1] = this.picoX;
    const y = boards.pico.length / 2;
    const top = this.picoFace + h.picoPcbThickness;
    // The display's header cut past its plastic, its wires soldered on.
    const displayCut = intersection(
      this.displayed(display(c, this.faceplate)),
      box(-50, 50, -50, 50, -1, this.plate + HEADER_PLASTIC + 1),
    );
    return union(
      this.shell.color("gold"),
      displayCut,
      box(x0, x1, -y, y, this.picoFace, top).color("green"),
      ...[-1, 1].map((sy) =>
        box(
          x0 + 1,
          x1 - 1,
          (sy * PICO_ROWS_APART) / 2 - 1.27,
          (sy * PICO_ROWS_APART) / 2 + 1.27,
          this.picoFace - h.picoHeaderHeight,
          this.picoFace,
        ).color("black"),
      ),
      box(x0 - h.picoUsbOverhang, x0 + 4, -4, 4, top, top + 2.6).color("silver"),
      this.lid.color("silver"),
    );
  }
}
