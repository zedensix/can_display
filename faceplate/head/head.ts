// The head: the display in landscape with the Pico W right behind it, in
// the smallest box they go in, for up high (the meter hood's top). The
// rest stays out of sight on a cable: the MCP2515 and the converter at
// the OBD port, the encoder wherever the hand falls. The display's SPI
// (62.5 MHz) won't go down a long cable, the MCP2515's (1 MHz) and the
// encoder will: so the Pico sits behind the display.
//
// - The shell: the front with the display's window and pockets (the
//   faceplate's, turned to landscape), 4 pins through the display board's
//   holes, melted into heads with the soldering iron; walls just around the
//   board, nothing standing into the room (the board goes in from the back,
//   as wide as the room); a notch in the end by the Pico's USB socket
//   for the plug, whichever way up the Pico lies, 2 slots for the lid's
//   hooks; its bottom takes 3M VHB tape, no wedge: the hood's slope
//   already leans it back.
// - The lid lies on the walls' back edge all round, 2 snap tabs hanging
//   along the top and bottom walls, their hooks clicking into the slots; a
//   small screwdriver through a slot pushes its hook back in. The Pico
//   stands on it, either way up, on 4 tall standoffs, their pins through
//   its holes melted into heads, each cut flat inside for the USB socket.
//   Vents around them, and the cable's slot, out the back.
//
// Seen from the back like the faceplate: x across, y up, the front at
// z = 0, the display board centered. Both print with no supports: the
// shell front down (the window's bevel, the slots' short bridges), the lid
// outside down (the hooks' 1.3 mm ledges, short enough to print in the air).

import type { ScadObject } from "scad-js";
import { cylinder, difference, hull, intersection, square, union } from "scad-js";
import type { Faceplate } from "../plate/faceplate";
import type { Config } from "../shared/config";
import { box, HEADER_PLASTIC, PICO_ROWS_APART, PITCH } from "../shared/geometry";
import { display } from "../shared/mockups";
import { cellHoles, honeycomb, type Rect } from "../shared/vents";

export class Head {
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
  readonly cable: [number, number];

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
    // Under the Pico, toward the hood.
    this.cable = [this.halfX / 2, -(boards.pico.length / 2 + this.halfY) / 2];

    if (this.halfY - boards.pico.length / 2 < h.cableHole + 1) {
      throw new Error("head: no room for the cable's slot under the Pico: check head.cableHole");
    }
    if (
      this.cable[0] - h.cableSlotLength / 2 < h.tabWidth / 2 + 1 ||
      this.cable[0] + h.cableSlotLength / 2 > this.halfX - 1
    ) {
      throw new Error("head: the cable's slot reaches a tab or the wall: check head.cableSlotLength");
    }
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
  private get usbGap(): number {
    return this.c.boards.pico.usbSocketWidth + 2 * this.c.clearance;
  }

  /** The Pico's standoffs, each cut flat inside to leave usbGap between them. */
  private get standoffs(): ScadObject {
    const { head: h } = this.c;
    const base = this.lidFace - h.picoStandoff;
    const g = this.usbGap / 2;
    return union(
      ...this.picoHoles.map(([x, y]) =>
        difference(
          this.stakes([[x, y]], h.standoffDiameter, base, this.lidFace + 0.01),
          box(x - 5, x + 5, -g, g, base - 1, this.lidFace + 1),
        ),
      ),
    );
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

  /** The outline's half sizes, the walls included. */
  private get outerX(): number {
    return this.halfX + this.c.head.wall;
  }

  private get outerY(): number {
    return this.halfY + this.c.head.wall;
  }

  /** y0..y1 on the top wall's side (sy = 1), mirrored onto the bottom's (sy = -1). */
  private side(sy: number, y0: number, y1: number): [number, number] {
    return sy > 0 ? [y0, y1] : [-y1, -y0];
  }

  /** Pins through a board's holes, d across, from z0 to z1. */
  private stakes(holes: [number, number][], d: number, z0: number, z1: number): ScadObject {
    return union(...holes.map(([x, y]) => cylinder(z1 - z0, d / 2, { center: false }).translate([x, y, z0])));
  }

  /** The shell, as built: front at z = 0, its walls up to the lid. */
  get shell(): ScadObject {
    const { head: h, display: d, clearance } = this.c;
    const f = this.faceplate;
    const socketZ = this.picoFace - 1.3;
    const boardFront = this.plate - d.pcbThickness;
    return union(
      difference(
        this.rounded(this.outerX, this.outerY, h.cornerRadius, 0, this.lidFace),
        box(-this.halfX, this.halfX, -this.halfY, this.halfY, this.plate, this.lidFace + 1),
        this.displayed(union(f.displayWindow, f.displayPockets)),
        // The USB plug's notch, up to the lid: either way up.
        box(
          -this.outerX - 1,
          -this.halfX + 0.01,
          -h.usbSlotWidth / 2,
          h.usbSlotWidth / 2,
          socketZ - h.usbSlotHeight / 2,
          this.lidFace + 1,
        ),
        // The hooks' slots, through the wall.
        ...[-1, 1].map((sy) =>
          box(
            -h.tabWidth / 2 - clearance,
            h.tabWidth / 2 + clearance,
            ...this.side(sy, this.halfY - 0.01, this.outerY + 1),
            this.tabTip - 0.3,
            this.tabTip + h.hookLength + 0.15,
          ),
        ),
      ),
      this.stakes(this.displayHoles, d.holeDiameter - 2 * clearance, boardFront - 0.01, this.plate + h.stakeLength),
    );
  }

  /** The 2 snap tabs, hanging from the lid along the top and bottom walls, a hook ramping out at each tip. */
  private get tabs(): ScadObject {
    const { head: h, clearance } = this.c;
    const w = h.tabWidth / 2;
    const outer = this.halfY - clearance;
    return union(
      ...[-1, 1].map((sy) =>
        union(
          box(-w, w, ...this.side(sy, outer - h.tabThickness, outer), this.tabTip, this.lidFace + 0.01),
          hull(
            box(-w, w, ...this.side(sy, outer - 0.01, outer), this.tabTip, this.tabTip + 0.01),
            box(
              -w,
              w,
              ...this.side(sy, outer - 0.01, this.halfY + h.hookDepth),
              this.tabTip + h.hookLength - 0.01,
              this.tabTip + h.hookLength,
            ),
          ),
        ),
      ),
    );
  }

  /** The lid, as built: from lidFace to the back, its tabs, the Pico's standoffs and their pins toward the front. */
  get lid(): ScadObject {
    const { head: h, boards, clearance } = this.c;
    const lx = this.halfX - clearance;
    const ly = this.halfY - clearance;
    const standoffR = h.standoffDiameter / 2;
    const keepOut: Rect[] = [
      ...this.picoHoles.map(([x, y]): Rect => [x - standoffR, x + standoffR, y - standoffR, y + standoffR]),
      ...[-1, 1].map(
        (sy): Rect => [-h.tabWidth / 2, h.tabWidth / 2, ...this.side(sy, ly - h.tabThickness - 1, ly + 1)],
      ),
      [
        this.cable[0] - h.cableSlotLength / 2,
        this.cable[0] + h.cableSlotLength / 2,
        this.cable[1] - h.cableHole / 2,
        this.cable[1] + h.cableHole / 2,
      ],
    ];
    const r = h.ventCell;
    const cells = honeycomb(r, h.ventWall, 0, Math.hypot(lx, ly), keepOut).filter(
      ([x, y]) => Math.abs(x) + r + 1 <= lx && Math.abs(y) + r + 1 <= ly,
    );
    const standoffBase = this.lidFace - h.picoStandoff;
    return difference(
      union(
        this.rounded(this.outerX, this.outerY, h.cornerRadius, this.lidFace, this.depth),
        this.tabs,
        this.standoffs,
        // Shorter by the USB socket, between them (stakeNearUsb).
        ...this.picoHoles.map(([x, y]) =>
          this.stakes(
            [[x, y]],
            boards.pico.holeDiameter - 2 * clearance,
            this.picoFace - (x < this.picoX[0] + boards.pico.width / 2 ? h.stakeNearUsb : h.stakeLength),
            standoffBase + 0.01,
          ),
        ),
      ),
      // The cable's slot, round at both ends.
      hull(
        ...[-1, 1].map((sx) =>
          cylinder(h.lidThickness + 2, h.cableHole / 2, { center: false }).translate([
            this.cable[0] + (sx * (h.cableSlotLength - h.cableHole)) / 2,
            this.cable[1],
            this.lidFace - 1,
          ]),
        ),
      ),
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
    const back = this.picoFace + h.picoPcbThickness;
    // The display's header cut past its plastic, its wires soldered on.
    const displayCut = intersection(
      this.displayed(display(c, this.faceplate)),
      box(-50, 50, -50, 50, -1, this.plate + HEADER_PLASTIC + 1),
    );
    return union(
      this.shell.color("gold"),
      displayCut,
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
      this.lid.color("silver"),
    );
  }
}
