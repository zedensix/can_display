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
// of the lid's hooks through it. The lid clips on with 3 pairs of tabs.
// Both parts print with no supports, the shell front down, the lid
// outside down.

import type { ScadObject } from "scad-js";
import { circle, cylinder, difference, intersection, square, union } from "scad-js";
import type { Faceplate } from "../plate/faceplate";
import { Case } from "../shared/case";
import type { Config } from "../shared/config";
import { box } from "../shared/geometry";
import { encoder } from "../shared/mockups";
import type { Rect } from "../shared/vents";

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
    const end = this.right + h.wall;
    this.sag = b.shroudRadius - Math.sqrt(b.shroudRadius ** 2 - end ** 2);

    if (e.boardLength / 2 > this.halfY || boards.mcp2515.length / 2 > this.halfY) {
      throw new Error("bar: the encoder's board or the MCP2515 is longer than the bar is tall");
    }
    if (b.frontWall + b.mcpStandoff + b.mcpPcbThickness + b.mcpTallest > this.lidFace) {
      throw new Error("bar: the MCP2515's terminal doesn't fit under the lid: check bar.mcpStandoff and mcpTallest");
    }
    if (b.shroudRadius <= end) {
      throw new Error("bar: bar.shroudRadius is shorter than half the bar");
    }
    if (b.endRoom < h.cableHole + 1) {
      throw new Error("bar: no room for the cable's slot past the MCP2515: check bar.endRoom");
    }
  }

  /** The faceplate's encoder cutouts or mockup, turned to sit past the display's -x end, its shaft at shaftX. */
  private encoderPlaced(o: ScadObject): ScadObject {
    return o.rotate([0, 0, 90]).translate([this.shaftX + this.faceplate.shaftY, 0, 0]);
  }

  private get mcpCenter(): number {
    return (this.mcp[0] + this.mcp[1]) / 2;
  }

  /** What hollows the bottom into an arc of bar.shroudRadius, its axis along z, tangent to the bottom wall: on the shroud. */
  private saddle(z0: number, z1: number): ScadObject {
    const r = this.c.bar.shroudRadius;
    return cylinder(z1 - z0 + 2, r, { center: false, $fn: 720 }).translate([0, -this.outerY - r, z0 - 1]);
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
        this.encoderPlaced(union(f.encoderCutouts, f.encoderScrewHoles)),
        this.hookSlots(this.tabXs, this.sag + 1),
      ),
      this.displayStakes,
      difference(this.encoderPlaced(f.encoderStandoffs), this.encoderPlaced(f.encoderScrewHoles)),
      this.mcpMount,
    );
  }

  /** The lid, as built: from lidFace to the back, its tabs, the Pico's standoffs. */
  get lid(): ScadObject {
    const { head: h, clearance } = this.c;
    const half = (h.cableSlotLength - h.cableHole) / 2;
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
      ]),
    );
  }

  /** Each part to print, as it lies on the bed. */
  get print(): Record<string, ScadObject> {
    return {
      "bar-shell": this.shell,
      "bar-lid": this.lid.rotate([180, 0, 0]).translate([0, 0, this.depth]),
      "bar-wireframe": this.wireframe,
    };
  }

  /** The outline seen from the front, the bottom's arc included. */
  private get outline(): ScadObject {
    const { head: h, bar: b } = this.c;
    const r = h.cornerRadius;
    const x = this.right + h.wall;
    return difference(
      square([2 * x - 2 * r, 2 * this.outerY + this.sag - 2 * r])
        .radius_offset(r)
        .translate([0, -this.sag / 2, 0]),
      circle(b.shroudRadius, { $fn: 720 }).translate([0, -this.outerY - b.shroudRadius, 0]),
    );
  }

  /**
   * A test print of the bar's length and its bottom's arc on the shroud:
   * its outline as a flat frame, bar.wireBar wide, bar.wireThickness thick.
   */
  get wireframe(): ScadObject {
    const b = this.c.bar;
    return difference(this.outline, this.outline.radius_offset(-b.wireBar)).linear_extrude(b.wireThickness);
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
