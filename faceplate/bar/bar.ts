// The bar: the modules in one long, thin box, laid along its length rather
// than stacked behind each other, the display in the middle. From the -x
// end (seen from the back): the encoder, its shaft out the front; room for
// the converter's micro-USB plug in the Pico's socket (the converter stays
// out, on the cable by the fuse box), under an opening in the lid its
// cable goes out through, and where a finger pulls it out to plug the
// computer in; the display in landscape with the Pico right behind it (the
// head's, shared/case.ts); the MCP2515 on 4 heat-staked standoffs, its
// header toward the display, its terminal toward the end, CAN-H and CAN-L
// out the back through a slot in the lid over it.
//
// The front is as thick as the display and the encoder need (the
// faceplate's) under them, and thin under the MCP2515. The bottom is
// hollowed into an arc to sit on the steering column's shroud, the slots
// of the lid's hooks through it. The lid clips on with 3 pairs of tabs,
// a lip under it inside the walls all round to stiffen it.
// Both parts print with no supports, the shell front down, the lid
// outside down.

import type { ScadObject } from "scad-js";
import { difference, intersection, polygon, union } from "scad-js";
import type { Faceplate } from "../plate/faceplate";
import { Case } from "../shared/case";
import type { Config } from "../shared/config";
import { box } from "../shared/geometry";
import { encoder } from "../shared/mockups";
import type { Rect } from "../shared/vents";

/**
 * Points along an arc through (0, 0), every mm from -half to half, sag
 * below it at both ends: the bar's bottom.
 */
function arc(sag: number, half: number): [number, number][] {
  const r = (half * half + sag * sag) / (2 * sag);
  const n = Math.ceil(half);
  return Array.from({ length: 2 * n + 1 }, (_, i): [number, number] => {
    const x = Math.max(-half, Math.min(half, i - n));
    return [x, -(r - Math.sqrt(r * r - x * x))];
  });
}

export class Bar extends Case {
  /** The room's ends, -right to right: the display in the middle. */
  readonly right: number;
  /** The encoder's shaft, on y = 0, and the MCP2515's ends along x. */
  readonly shaftX: number;
  readonly mcp: [number, number];
  /** The plug's room, from the encoder to the display. */
  readonly plugRoom: number;
  /** Where the lid's tabs are: over the plug, the display and the MCP2515. */
  readonly tabXs: number[];
  /** The cable's slot through the lid, across, over the end room. */
  readonly cableX: number;
  /** How far the bottom's arc stands under the bottom wall at the ends. */
  readonly sag: number;

  constructor(c: Config, faceplate: Faceplate) {
    super(c, faceplate);
    const { bar: b, boards, encoder: e, head: h } = c;
    // The encoder across the bar: its board's width, its standoffs, along x.
    const encoderHalf = Math.max(e.boardWidth / 2, e.holesFromShaft + e.standoffDiameter / 2);
    const side = Math.max(b.gap + boards.mcp2515.width + b.endRoom, b.plugRoomMin + 2 * encoderHalf + b.encoderEnd);
    this.right = this.halfX + side;
    this.mcp = [this.halfX + b.gap, this.halfX + b.gap + boards.mcp2515.width];
    this.shaftX = -this.right + b.encoderEnd + encoderHalf;
    this.plugRoom = -this.halfX - (this.shaftX + encoderHalf);
    const mcpCenter = (this.mcp[0] + this.mcp[1]) / 2;
    this.tabXs = [-mcpCenter, 0, mcpCenter];
    this.cableX = (this.mcp[1] + this.right) / 2;
    this.sag = b.shroudSag;

    if (e.boardLength / 2 > this.halfY || boards.mcp2515.length / 2 > this.halfY) {
      throw new Error("bar: the encoder's board or the MCP2515 is longer than the bar is tall");
    }
    if (b.frontWall + b.mcpStandoff + b.mcpPcbThickness + b.mcpTallest > this.lidFace) {
      throw new Error("bar: the MCP2515's terminal doesn't fit under the lid: check bar.mcpStandoff and mcpTallest");
    }
    if (b.endRoom < h.cableHole + 1) {
      throw new Error("bar: no room for the cable's slot past the MCP2515: check bar.endRoom");
    }
  }

  /** The faceplate's encoder cutouts or mockup, turned to sit past the display's -x end, its shaft at shaftX. */
  private encoderPlaced(o: ScadObject): ScadObject {
    return o.rotate([0, 0, 90]).translate([this.shaftX + this.faceplate.shaftY, 0, 0]);
  }

  /**
   * Pins through the encoder board's 2 holes, on the 2 standoffs above its
   * shaft (in the faceplate's frame): the other 2 hold the board level.
   * The board goes on with its holes on the pins' side.
   */
  private get encoderStakes(): ScadObject {
    const { encoder: e, head: h, clearance } = this.c;
    const f = this.faceplate;
    const holes = f.encoderBoardStandoffs.filter(([, y]) => y > f.shaftY);
    const z0 = f.encoderBoardFace - 0.01;
    return this.encoderPlaced(
      this.stakes(holes, e.holeDiameter - 2 * clearance, z0, f.encoderBoardFace + e.pcbThickness + h.stakeLength),
    );
  }

  private get mcpCenter(): number {
    return (this.mcp[0] + this.mcp[1]) / 2;
  }

  /**
   * What hollows the bottom into an arc, bar.shroudSag deeper at the ends,
   * its axis along z, tangent to the bottom wall: on the shroud. A polygon
   * through the arc every mm (a circle that large would be a few long
   * flats), and everything under it.
   */
  private get underArc(): ScadObject {
    const x = this.right + this.c.head.wall;
    const low = -this.outerY - this.sag - 10;
    return polygon([
      [-x - 1, -this.outerY - this.sag],
      ...arc(this.sag, x).map(([px, py]): [number, number] => [px, py - this.outerY]),
      [x + 1, -this.outerY - this.sag],
      [x + 1, low],
      [-x - 1, low],
    ]);
  }

  private saddle(z0: number, z1: number): ScadObject {
    return this.underArc.linear_extrude(z1 - z0 + 2).translate([0, 0, z0 - 1]);
  }

  /** The opening in the lid over the plug, head.usbSlotWidth across. */
  private get plugOpening(): [[number, number], [number, number]] {
    const w = this.c.head.usbSlotWidth;
    return [
      [-this.halfX - this.plugRoom + w / 2 + 1, 0],
      [-this.halfX - w / 2, 0],
    ];
  }

  /** The MCP2515's 4 standoffs on the front, and their pins through its holes. */
  private get mcpMount(): ScadObject {
    const { bar: b, boards, head: h, clearance } = this.c;
    const m = boards.mcp2515;
    const holes = [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [this.mcpCenter + (sx * m.holesApartX) / 2, (sy * m.holesApartY) / 2]),
    );
    const top = b.frontWall + b.mcpStandoff;
    return union(
      this.stakes(holes, b.mcpStandoffDiameter, b.frontWall - 0.01, top),
      this.stakes(holes, m.holeDiameter - 2 * clearance, top - 0.01, top + b.mcpPcbThickness + h.stakeLength),
    );
  }

  /** The shell, as built: front at z = 0, its walls up to the lid. */
  get shell(): ScadObject {
    const f = this.faceplate;
    const { bar: b, head: h } = this.c;
    return union(
      difference(
        this.rounded(-this.right - h.wall, this.right + h.wall, this.outerY, 0, this.lidFace, this.sag),
        this.saddle(0, this.lidFace),
        box(-this.right, this.mcp[0] - b.gap / 2, -this.halfY, this.halfY, this.plate, this.lidFace + 1),
        box(this.mcp[0] - b.gap / 2 - 0.01, this.right, -this.halfY, this.halfY, b.frontWall, this.lidFace + 1),
        this.displayed(union(f.displayWindow, f.displayPockets)),
        this.encoderPlaced(f.encoderCutouts),
        this.hookSlots(this.tabXs, this.sag + 1),
      ),
      this.displayStakes,
      this.encoderPlaced(f.encoderStandoffs),
      this.encoderStakes,
      this.mcpMount,
    );
  }

  /** The lid's lip, inside the walls all round, broken around each tab so it still bends. */
  private get lip(): ScadObject {
    const { bar: b, head: h, clearance } = this.c;
    const lx = this.right - clearance;
    const ly = this.halfY - clearance;
    const w = b.lipWidth;
    const z0 = this.lidFace - b.lipHeight;
    return difference(
      box(-lx, lx, -ly, ly, z0, this.lidFace + 0.01),
      box(-lx + w, lx - w, -ly + w, ly - w, z0 - 1, this.lidFace + 1),
      ...this.tabXs.map((x) =>
        box(x - h.tabWidth / 2 - 1, x + h.tabWidth / 2 + 1, -ly - 1, ly + 1, z0 - 1, this.lidFace + 1),
      ),
    );
  }

  /** The lid, as built: from lidFace to the back, its lip, its tabs, the Pico's standoffs. */
  get lid(): ScadObject {
    const { bar: bc, head: h, clearance } = this.c;
    const half = (h.cableSlotLength - h.cableHole) / 2;
    const lx = this.right - clearance;
    const ly = this.halfY - clearance;
    const lw = bc.lipWidth;
    const [a, b] = this.plugOpening;
    const w = h.usbSlotWidth;
    const slot: Rect = [
      this.cableX - h.cableHole / 2,
      this.cableX + h.cableHole / 2,
      -h.cableSlotLength / 2,
      h.cableSlotLength / 2,
    ];
    return difference(
      union(
        // The shell's outline, its bottom's arc included.
        difference(
          this.rounded(-this.right - h.wall, this.right + h.wall, this.outerY, this.lidFace, this.depth, this.sag),
          this.saddle(this.lidFace, this.depth),
        ),
        this.lip,
        this.tabs(this.tabXs),
        this.picoMount,
      ),
      this.cableSlot([this.cableX, -half], [this.cableX, half]),
      this.cableSlot(a, b, w),
      this.vents(-this.right + clearance, this.right - clearance, [
        ...this.picoKeepOut,
        ...this.tabsKeepOut(this.tabXs),
        slot,
        [a[0] - w / 2, b[0] + w / 2, -w / 2, w / 2],
        // The lip's 4 sides.
        [-lx, lx, ly - lw, ly],
        [-lx, lx, -ly, -ly + lw],
        [-lx, -lx + lw, -ly, ly],
        [lx - lw, lx, -ly, ly],
      ]),
    );
  }

  /** Each part to print, as it lies on the bed. */
  get print(): Record<string, ScadObject> {
    return {
      "bar-shell": this.shell,
      "bar-lid": this.lid.rotate([180, 0, 0]).translate([0, 0, this.depth]),
      "bar-arcs": this.arcGauge,
    };
  }

  /**
   * A test print to pick the bottom's arc: a strip as long as the bar for
   * each of bar.gaugeSags, its bottom that much deeper at the ends, as many
   * notches on its top as its rank. Flat on the bed.
   */
  get arcGauge(): ScadObject {
    const { bar: b, head: h } = this.c;
    const half = this.right + h.wall;
    const pitch = b.gaugeHeight + Math.max(...b.gaugeSags) + 3;
    return union(
      ...b.gaugeSags.map((sag, i) =>
        difference(
          polygon([...arc(sag, half), [half, b.gaugeHeight], [-half, b.gaugeHeight]]).linear_extrude(b.gaugeThickness),
          ...Array.from({ length: i + 1 }, (_, k) =>
            box(
              k * 4 - i * 2 - 0.75,
              k * 4 - i * 2 + 0.75,
              b.gaugeHeight - 2,
              b.gaugeHeight + 1,
              -1,
              b.gaugeThickness + 1,
            ),
          ),
        ).translate([0, i * pitch, 0]),
      ),
    );
  }

  /** The MCP2515, components toward the lid, its header toward the display, its terminal toward the end. */
  private get mcpMockup(): ScadObject {
    const { bar: b, boards } = this.c;
    const m = boards.mcp2515;
    const [x0, x1] = this.mcp;
    const base = b.frontWall + b.mcpStandoff;
    const face = base + b.mcpPcbThickness;
    return union(
      box(x0, x1, -m.length / 2, m.length / 2, base, face).color("navy"),
      box(x1 - 7.5, x1, -5, 5, face, face + b.mcpTallest).color("deepskyblue"), // terminal: H, L
      box(x0 + 1.5, x0 + 4, -8.9, 8.9, face, face + 2.5).color("black"), // header: 7 pins
      box(this.mcpCenter - 2, this.mcpCenter + 6, -4, 4, face, face + 1.5).color("black"), // MCP2515
      box(this.mcpCenter - 3, this.mcpCenter + 8, 6, 10.5, face, face + 3.5).color("silver"), // 8 MHz crystal
    );
  }

  /** Every module in place, the lid off: for the pictures, never printed. */
  get inside(): ScadObject {
    const { c } = this;
    const socket = this.picoX[0] - c.head.picoUsbOverhang;
    return union(
      this.shell.color("gold"),
      this.stackMockup,
      // The encoder's bent pins cut at the walls: their wires are soldered on.
      intersection(this.encoderPlaced(encoder(c, this.faceplate)), box(-500, 500, -this.halfY, this.halfY, -50, 50)),
      // The converter's plug in the Pico's socket.
      box(socket - 18, socket, -5.5, 5.5, this.picoFace - 4.8, this.picoFace + 2.2).color("black"),
      this.mcpMockup,
    );
  }

  /** Everything in place, the lid on: for the pictures, never printed. */
  get assembly(): ScadObject {
    return union(this.inside, this.lid.color("silver"));
  }
}
