// What the head and the bar share: the display in landscape with the Pico
// W right behind it, in a box whose lid clips on. The front holds the
// display as the faceplate does (its window and pockets, turned to
// landscape) on 4 pins through the board's holes, melted into heads with
// the soldering iron. The lid lies on the walls' back edge all round, snap
// tabs hanging along the top and bottom walls, their hooks clicking into
// slots through them; a small screwdriver through a slot pushes its hook
// back in. The Pico stands on the lid, either way up, on 4 standoffs, their
// pins through its holes melted into heads, each cut flat inside for the
// USB socket. Vents in the lid.
//
// Their dimensions are the head's (head/config.ts). Seen from the back like
// the faceplate: x across, y up, the front at z = 0, the display board
// centered, the Pico's USB socket at its -x end. Both parts print with no
// supports: the shell front down, the lid outside down (the hooks' 1.8 mm
// ledges, short enough to print in the air).

import type { ScadObject } from "scad-js";
import { cylinder, difference, hull, intersection, square, union } from "scad-js";
import type { Faceplate } from "../plate/faceplate";
import type { Config } from "./config";
import { box, HEADER_PLASTIC, PICO_ROWS_APART, PITCH } from "./geometry";
import { display } from "./mockups";
import { cellHoles, honeycomb, type Rect } from "./vents";

export class Case {
  /** The room inside the walls: the display board and its clearance. */
  readonly halfX: number;
  readonly halfY: number;
  /** The front's thickness: the display board's back. */
  readonly plate: number;
  /** The Pico's board, its components' side (toward the display), and the lid's inner face, where the walls end. */
  readonly picoFace: number;
  readonly lidFace: number;
  readonly depth: number;
  /** The Pico's board along x, its USB socket at the left end (seen from the back). */
  readonly picoX: [number, number];
  readonly picoHoles: [number, number][];
  readonly displayHoles: [number, number][];
  /** Where each tab ends, its hook's ramp starting there. */
  readonly tabTip: number;

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { display: d, head: h, boards, clearance } = c;
    this.halfX = d.pcbLength / 2 + clearance;
    this.halfY = d.pcbWidth / 2 + clearance;
    this.plate = faceplate.thickness;
    this.picoFace = this.plate + h.picoComponents;
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
    // Landscape: the board's length along x.
    this.displayHoles = [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [(sx * d.holesApartY) / 2, (sy * d.holesApartX) / 2]),
    );
    this.tabTip = this.lidFace - h.tabLength;

    const standoffR = h.standoffDiameter / 2;
    if (boards.pico.holesApartY / 2 + standoffR + clearance > PICO_ROWS_APART / 2 - PITCH / 2) {
      throw new Error("head: the Pico's standoffs run into its header's plastic: check head.standoffDiameter");
    }
    if (boards.pico.holesApartY / 2 - boards.pico.holeDiameter / 2 + clearance < this.usbGap / 2) {
      throw new Error("head: the Pico's pins by its USB socket leave under boards.pico.usbSocketWidth between them");
    }
    if (this.tabTip < this.plate + 0.5) {
      throw new Error("head: the lid's tabs reach the display's board: check head.tabLength");
    }
    if (this.halfY - clearance - h.tabThickness < boards.pico.length / 2) {
      throw new Error("head: the lid's tabs reach the Pico: check head.tabThickness");
    }
  }

  /** Room between the standoffs across: the USB socket and its clearance, whichever way up the Pico goes. */
  protected get usbGap(): number {
    return this.c.boards.pico.usbSocketWidth + 2 * this.c.clearance;
  }

  /** The Pico's standoffs on the lid, each cut flat inside to leave usbGap between them, and their pins. */
  protected get picoMount(): ScadObject {
    const { head: h, boards, clearance } = this.c;
    const base = this.lidFace - h.picoStandoff;
    const g = this.usbGap / 2;
    return union(
      ...this.picoHoles.map(([x, y]) =>
        difference(
          this.stakes([[x, y]], h.standoffDiameter, base, this.lidFace + 0.01),
          box(x - 5, x + 5, -g, g, base - 1, this.lidFace + 1),
        ),
      ),
      // Shorter by the USB socket, between them (stakeNearUsb).
      ...this.picoHoles.map(([x, y]) =>
        this.stakes(
          [[x, y]],
          boards.pico.holeDiameter - 2 * clearance,
          this.picoFace - (x < this.picoX[0] + boards.pico.width / 2 ? h.stakeNearUsb : h.stakeLength),
          base + 0.01,
        ),
      ),
    );
  }

  /** The keep-out rects of the Pico's standoffs, for the lid's vents. */
  protected get picoKeepOut(): Rect[] {
    const r = this.c.head.standoffDiameter / 2;
    return this.picoHoles.map(([x, y]): Rect => [x - r, x + r, y - r, y + r]);
  }

  /** The faceplate's display cutouts or mockup, turned to landscape, the board centered. */
  protected displayed(o: ScadObject): ScadObject {
    return o.rotate([0, 0, -90]).translate([this.c.display.pcbLength / 2, 0, 0]);
  }

  /** A rounded rectangle, x0..x1 by -halfY..halfY (its bottom lowered by `below`), extruded from z0 to z1. */
  protected rounded(x0: number, x1: number, halfY: number, z0: number, z1: number, below = 0): ScadObject {
    const r = this.c.head.cornerRadius;
    return square([x1 - x0 - 2 * r, 2 * halfY + below - 2 * r])
      .radius_offset(r)
      .linear_extrude(z1 - z0)
      .translate([(x0 + x1) / 2, -below / 2, z0]);
  }

  /** The outline's half sizes, the walls included. */
  protected get outerX(): number {
    return this.halfX + this.c.head.wall;
  }

  protected get outerY(): number {
    return this.halfY + this.c.head.wall;
  }

  /** y0..y1 on the top wall's side (sy = 1), mirrored onto the bottom's (sy = -1). */
  protected side(sy: number, y0: number, y1: number): [number, number] {
    return sy > 0 ? [y0, y1] : [-y1, -y0];
  }

  /** Pins through a board's holes, d across, from z0 to z1. */
  protected stakes(holes: [number, number][], d: number, z0: number, z1: number): ScadObject {
    return union(...holes.map(([x, y]) => cylinder(z1 - z0, d / 2, { center: false }).translate([x, y, z0])));
  }

  /** The notch for the Pico's USB plug through the -x end wall, up to the lid: either way up. */
  protected get usbNotch(): ScadObject {
    const h = this.c.head;
    const socketZ = this.picoFace - 1.3;
    return box(
      -this.outerX - 1,
      -this.halfX + 0.01,
      -h.usbSlotWidth / 2,
      h.usbSlotWidth / 2,
      socketZ - h.usbSlotHeight / 2,
      this.lidFace + 1,
    );
  }

  /** The pins through the display board's holes, from its front. */
  protected get displayStakes(): ScadObject {
    const { head: h, display: d, clearance } = this.c;
    const boardFront = this.plate - d.pcbThickness;
    return this.stakes(
      this.displayHoles,
      d.holeDiameter - 2 * clearance,
      boardFront - 0.01,
      this.plate + h.stakeLength,
    );
  }

  /** The slots the lid's hooks click into, through the top and bottom walls (and out past them by `out`), a pair at each x. */
  protected hookSlots(xs: number[], out = 1): ScadObject {
    const { head: h, clearance } = this.c;
    return union(
      ...xs.flatMap((x) =>
        [-1, 1].map((sy) =>
          box(
            x - h.tabWidth / 2 - clearance,
            x + h.tabWidth / 2 + clearance,
            ...this.side(sy, this.halfY - 0.01, this.outerY + out),
            this.tabTip - 0.3,
            // Room over the hook's catch for the slot's top, a bridge, sagging.
            this.tabTip + h.hookLength + 0.3,
          ),
        ),
      ),
    );
  }

  /** The snap tabs, a pair at each x hanging from the lid along the top and bottom walls, a hook ramping out at each tip. */
  protected tabs(xs: number[]): ScadObject {
    const { head: h, clearance } = this.c;
    const w = h.tabWidth / 2;
    const outer = this.halfY - clearance;
    return union(
      ...xs.flatMap((x) =>
        [-1, 1].map((sy) =>
          union(
            box(x - w, x + w, ...this.side(sy, outer - h.tabThickness, outer), this.tabTip, this.lidFace + 0.01),
            hull(
              box(x - w, x + w, ...this.side(sy, outer - 0.01, outer), this.tabTip, this.tabTip + 0.01),
              box(
                x - w,
                x + w,
                ...this.side(sy, outer - 0.01, this.halfY + h.hookDepth),
                this.tabTip + h.hookLength - 0.01,
                this.tabTip + h.hookLength,
              ),
            ),
          ),
        ),
      ),
    );
  }

  /** The keep-out rects of the tabs at each x, for the lid's vents. */
  protected tabsKeepOut(xs: number[]): Rect[] {
    const { head: h, clearance } = this.c;
    const ly = this.halfY - clearance;
    return xs.flatMap((x) =>
      [-1, 1].map(
        (sy): Rect => [x - h.tabWidth / 2, x + h.tabWidth / 2, ...this.side(sy, ly - h.tabThickness - 1, ly + 1)],
      ),
    );
  }

  /** A slot through the lid, round at both ends, its ends' centers a and b, d across (head.cableHole by default). */
  protected cableSlot(a: [number, number], b: [number, number], d = this.c.head.cableHole): ScadObject {
    const h = this.c.head;
    return hull(
      ...[a, b].map(([x, y]) =>
        cylinder(h.lidThickness + 2, d / 2, { center: false }).translate([x, y, this.lidFace - 1]),
      ),
    );
  }

  /** The lid's honeycomb, from x0 to x1 inside the walls, clear of keepOut. */
  protected vents(x0: number, x1: number, keepOut: Rect[]): ScadObject {
    const { head: h, clearance } = this.c;
    const ly = this.halfY - clearance;
    const r = h.ventCell;
    const cells = honeycomb(r, h.ventWall, 0, Math.hypot(Math.max(-x0, x1), ly), keepOut).filter(
      ([x, y]) => x - r - 1 >= x0 && x + r + 1 <= x1 && Math.abs(y) + r + 1 <= ly,
    );
    return cellHoles(cells, r, this.lidFace - 1, this.depth + 1);
  }

  /** The display, its header cut past its plastic, and the Pico on its standoffs: for the pictures, never printed. */
  protected get stackMockup(): ScadObject {
    const { c } = this;
    const { boards, head: h } = c;
    const [x0, x1] = this.picoX;
    const y = boards.pico.length / 2;
    const back = this.picoFace + h.picoPcbThickness;
    return union(
      intersection(
        this.displayed(display(c, this.faceplate)),
        box(-50, 50, -50, 50, -1, this.plate + HEADER_PLASTIC + 1),
      ),
      box(x0, x1, -y, y, this.picoFace, back).color("green"),
      ...[-1, 1].map((sy) =>
        box(
          x0 + 1,
          x1 - 1,
          (sy * PICO_ROWS_APART) / 2 - 1.27,
          (sy * PICO_ROWS_APART) / 2 + 1.27,
          back,
          back + h.picoHeaderHeight,
        ).color("black"),
      ),
      box(x0 - h.picoUsbOverhang, x0 + 4, -4, 4, this.picoFace - 2.6, this.picoFace).color("silver"),
    );
  }
}
