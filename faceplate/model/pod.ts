// The pod: the whole dash in the driver's round air vent, where a 52 mm
// gauge sat in a printed adapter (faceplate/resources/gauge_vent.stl).
//
// - The face takes the adapter's outside (its body, the step down at the
//   back, the 2 ears at the front edge), closed at the front: the display
//   in landscape behind its window, the encoder under it, both held as on
//   the faceplate (its cutouts, turned and moved). Behind the face, a room
//   for their wires narrows at 45 degrees to a spigot on the back.
// - The can goes into the vent's duct, its front band onto the spigot,
//   held by 2 countersunk M3 screws; its back end has the cable's hole.
// - The sled slides into the can between 2 pairs of ribs, held between
//   the spigot and the can's back: the Pico and the MCP2515 on standoffs on
//   top, the converter zip-tied under it, notches in its edges for the
//   wires from one side to the other.
//
// Seen from the back like the faceplate: x across, y up, the front face at
// z = 0, z into the vent. Each part prints with no supports: the face
// front down (the room's 45 degree ceiling), the can on its back end, the
// sled flat.

import type { ScadObject } from "scad-js";
import { cylinder, difference, union } from "scad-js";
import type { Config } from "./config";
import type { Faceplate } from "./faceplate";
import { box } from "./geometry";
import { display, encoder } from "./mockups";
import { podSled } from "./podsled";
import { cellHoles, honeycomb, type Rect } from "./podvents";

/** A cylinder along x through x0..x1, centered on (y, z), radius r (or r0 at x0 to r1 at x1). */
function alongX(x0: number, x1: number, y: number, z: number, r: number | [number, number]): ScadObject {
  return cylinder(x1 - x0, r, { center: false })
    .rotate([0, 90, 0])
    .translate([x0, y, z]);
}

/** A cylinder along z through z0..z1, centered on (x, y). */
function alongZ(z0: number, z1: number, r: number | [number, number], x = 0, y = 0): ScadObject {
  return cylinder(z1 - z0, r, { center: false }).translate([x, y, z0]);
}

export class Pod {
  readonly back: number; // the face's back end, where the spigot starts
  readonly spigotIn: number;
  readonly spigotOut: number;
  readonly screwZ: number;
  readonly canIn: number;
  readonly bandIn: number;
  readonly plateFront: number;
  readonly plateEnd: number;
  readonly canEnd: number;
  /** The boards along the sled: [front, back] in z. */
  readonly pico: [number, number];
  readonly mcp: [number, number];
  readonly converter: [number, number];

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { pod, clearance, boards } = c;
    this.back = pod.bodyLength + pod.stepLength + pod.backLength;
    this.canIn = pod.canDiameter / 2 - pod.canWall;
    this.bandIn = pod.canDiameter / 2 - pod.canBandWall;
    this.spigotOut = this.bandIn - clearance;
    this.spigotIn = this.spigotOut - pod.spigotWall;
    this.screwZ = this.back + pod.spigotLength / 2;
    this.plateFront = this.back + pod.spigotLength + clearance;
    // The Pico's USB socket toward the front, a little past its board.
    this.pico = [this.plateFront + pod.gap + 1, this.plateFront + pod.gap + 1 + boards.pico.width];
    this.mcp = [this.pico[1] + pod.gap, this.pico[1] + pod.gap + boards.mcp2515.width];
    this.converter = [this.pico[0], this.pico[0] + c.converter.length];
    this.plateEnd = this.mcp[1] + 2;
    this.canEnd = this.plateEnd + clearance + pod.capThickness;
    if (this.converter[1] > this.plateEnd) throw new Error("pod: the converter is longer than the sled");
  }

  /** The faceplate's display cutouts or mockup, turned to landscape and moved to the pod's frame. */
  private displayed(o: ScadObject): ScadObject {
    const { display: d, pod } = this.c;
    const activeMiddle = this.faceplate.activeTop - d.activeLength / 2;
    return o.rotate([0, 0, -90]).translate([-activeMiddle, pod.displayY, 0]);
  }

  /** The faceplate's encoder cutouts or mockup, moved under the display. */
  private encoderMoved(o: ScadObject): ScadObject {
    return o.translate([0, this.c.pod.shaftY - this.faceplate.shaftY, 0]);
  }

  /** The adapter's outside: body, step, back end, and the 2 ears at the front edge. */
  private get body(): ScadObject {
    const { pod } = this.c;
    const r = pod.bodyDiameter / 2;
    const rb = pod.backDiameter / 2;
    const z1 = pod.bodyLength + pod.stepLength;
    return union(
      alongZ(0, pod.bodyLength, r),
      alongZ(pod.bodyLength - 0.01, z1, [r, rb]),
      alongZ(z1 - 0.01, this.back, rb),
      ...[-1, 1].map((side) =>
        box(
          side < 0 ? -r - pod.earOut : r - 0.5,
          side < 0 ? -r + 0.5 : r + pod.earOut,
          -pod.earWidth / 2,
          pod.earWidth / 2,
          0,
          pod.earThickness,
        ),
      ),
    );
  }

  /** The room behind the face: from the face's back, narrowing at 45 degrees to the spigot's bore. */
  private get room(): ScadObject {
    const { pod } = this.c;
    const t = this.faceplate.thickness;
    const widest = pod.bodyDiameter / 2 - pod.wall;
    const atFace = Math.min(widest, this.spigotIn + (this.back - t));
    return union(
      alongZ(t, this.back + 0.01, [atFace, this.spigotIn]),
      alongZ(this.back - 0.01, this.back + pod.spigotLength + 1, this.spigotIn),
    );
  }

  /** The spigot the can's band goes onto, a pilot hole each side for the can's screws. */
  private get spigot(): ScadObject {
    const { pod, screw } = this.c;
    return difference(
      alongZ(this.back - 0.01, this.back + pod.spigotLength, this.spigotOut),
      alongZ(this.back - 1, this.back + pod.spigotLength + 1, this.spigotIn),
      alongX(-this.spigotOut - 1, this.spigotOut + 1, 0, this.screwZ, screw.m3PilotDiameter / 2),
    );
  }

  /**
   * The face's vents: through the front into the room, around the display's
   * board and the encoder's; and through the back, around the can, into the
   * room's 45 degree ceiling.
   */
  private get vents(): ScadObject {
    const { pod, display: d, encoder: e, clearance } = this.c;
    const f = this.faceplate;
    const t = f.thickness;
    const shift = -(f.activeTop - d.activeLength / 2);
    // Symmetric across, as wide as the board's wider side: an even honeycomb.
    const half = Math.max(d.pcbLength + clearance - shift, clearance + shift);
    const board: Rect = [
      -half,
      half,
      pod.displayY - d.pcbWidth / 2 - clearance,
      pod.displayY + d.pcbWidth / 2 + clearance,
    ];
    const knob: Rect = [
      f.encoderBoardLeft,
      f.encoderBoardRight,
      pod.shaftY - e.boardWidth / 2 - clearance,
      pod.shaftY + e.boardWidth / 2 + clearance,
    ];
    const roomAtFace = Math.min(pod.bodyDiameter / 2 - pod.wall, this.spigotIn + (this.back - t));
    const r = pod.ventCell;
    const front = honeycomb(r, pod.ventWall, 0, roomAtFace - 1, [board, knob]);
    // Clear of the can's front end against the back.
    const back = honeycomb(r, pod.ventWall, pod.canDiameter / 2 + 0.5, pod.backDiameter / 2 - 1.5);
    return union(cellHoles(front, r, -1, t + 1), cellHoles(back, r, t + 1, this.back + 1));
  }

  /** The face, as built: front at z = 0. */
  get face(): ScadObject {
    const f = this.faceplate;
    return union(
      difference(
        union(difference(this.body, this.room), this.encoderMoved(f.encoderStandoffs)),
        this.displayed(union(f.displayWindow, f.displayPockets, f.displayScrewHoles)),
        this.encoderMoved(union(f.encoderCutouts, f.encoderScrewHoles)),
        this.vents,
      ),
      this.spigot,
    );
  }

  /** The can, as built: its front band at z = back, onto the spigot. */
  get can(): ScadObject {
    const { pod, clearance } = this.c;
    const r = pod.canDiameter / 2;
    const bandEnd = this.back + pod.canBandLength;
    const inner = this.canEnd - pod.capThickness;
    const ribY0 = pod.plateThickness / 2 + clearance;
    const head = pod.screwHead / 2;
    const cableY = (pod.canDiameter / 2 - pod.cableHole / 2) / 2 + 1;
    return difference(
      union(
        difference(
          alongZ(this.back, this.canEnd, r),
          alongZ(this.back - 1, bandEnd + 0.01, this.bandIn),
          alongZ(bandEnd, inner, this.canIn),
        ),
        // The ribs the sled's edges slide between.
        ...[-1, 1].flatMap((sx) =>
          [-1, 1].map((sy) => {
            const [x0, x1] =
              sx < 0 ? [-this.canIn - 0.5, -this.canIn + pod.ribDepth] : [this.canIn - pod.ribDepth, this.canIn + 0.5];
            const [y0, y1] = sy < 0 ? [-ribY0 - pod.ribWidth, -ribY0] : [ribY0, ribY0 + pod.ribWidth];
            return box(x0, x1, y0, y1, bandEnd, inner + 0.01);
          }),
        ),
      ),
      alongZ(inner - 1, this.canEnd + 1, pod.cableHole / 2, 0, cableY),
      // The vents through its back end, where the duct's air comes from, clear of the cable's hole and the sled's end.
      cellHoles(
        honeycomb(pod.ventCell, pod.ventWall, 0, this.canIn - 1, [
          [-pod.cableHole / 2, pod.cableHole / 2, cableY - pod.cableHole / 2, cableY + pod.cableHole / 2],
          [-this.canIn, this.canIn, -pod.plateThickness / 2 - clearance, pod.plateThickness / 2 + clearance],
        ]),
        pod.ventCell,
        inner - 1,
        this.canEnd + 1,
      ),
      // The screws' holes, countersunk at 90 degrees.
      ...[-1, 1].map((side) =>
        union(
          alongX(-r - 1, r + 1, 0, this.screwZ, 1.7),
          side < 0
            ? alongX(-r - 0.01, -r + head - 1.7, 0, this.screwZ, [head, 1.7])
            : alongX(r - head + 1.7, r + 0.01, 0, this.screwZ, [1.7, head]),
        ),
      ),
    );
  }

  /** The sled, as built: the plate across the can at y = 0, from plateFront to plateEnd (podsled.ts). */
  get sled(): ScadObject {
    return podSled(this);
  }

  /** Each part to print, as it lies on the bed. */
  get print(): Record<string, ScadObject> {
    const t = this.c.pod.plateThickness / 2;
    return {
      "pod-face": this.face,
      "pod-can": this.can.rotate([180, 0, 0]).translate([0, 0, this.canEnd]),
      "pod-sled": this.sled.rotate([90, 0, 0]).translate([0, 0, t]),
    };
  }

  /** Everything in place, the can cut open on the right: for the pictures, never printed. */
  get assembly(): ScadObject {
    const { c } = this;
    const { boards, converter: k, pod } = c;
    const top = pod.plateThickness / 2;
    const picoBase = top + boards.pico.standoffHeight;
    const mcpBase = top + boards.mcp2515.standoffHeight;
    const r = pod.canDiameter / 2;
    return union(
      this.face.color("gold"),
      this.displayed(display(c, this.faceplate)),
      this.encoderMoved(encoder(c, this.faceplate)),
      difference(this.can, box(0, r + 1, -r - 1, r + 1, this.back - 1, this.canEnd + 1)).color("silver"),
      this.sled.color("tomato"),
      box(-boards.pico.length / 2, boards.pico.length / 2, picoBase, picoBase + 1, ...this.pico).color("green"),
      box(-boards.mcp2515.length / 2, boards.mcp2515.length / 2, mcpBase, mcpBase + 1.6, ...this.mcp).color("navy"),
      box(-5, 5, mcpBase + 1.6, mcpBase + 11.6, this.mcp[1] - 7.5, this.mcp[1]).color("deepskyblue"),
      box(-k.width / 2, k.width / 2, -top - k.height, -top, ...this.converter).color("dimgray"),
    );
  }
}
