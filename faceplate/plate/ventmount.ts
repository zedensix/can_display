// A temporary mount on an air vent with horizontal slats, to try the dash
// on the road: not the final enclosure.
//
// The clamp takes one slat in a sandwich. Its upper leg comes out of a bar
// across the tray's back between the 2 top posts (clear of the Pico below
// it, over the rim along the top edge), held by the 2 top tray screws,
// longer. The lower leg, the jaw, is a part of its own: an M3 screw through
// the upper leg bites into it, and the thumbwheel on the screw's head, in
// the room between the tray and the vent, pulls it up against the slat.
// The jaw's back end bears on the upper leg, so it closes like pliers. The
// feet, one on each bottom post, rest on what's under the vent (the vent is
// shorter than the dash), so the dash stands square and doesn't turn with
// the slat.
//
// Built with the tray's back at z = 0 and the faceplate's X and Y. Each
// part prints flat, with no supports: the bar and its leg on their top
// face, the jaw on its side, the wheel and the feet standing.

import type { ScadObject } from "scad-js";
import { cylinder, difference, union } from "scad-js";
import type { Config } from "../shared/config";
import { box } from "../shared/geometry";
import type { Faceplate } from "./faceplate";

const BAR_THICKNESS = 4;
/** Room between the bar and the Pico's top edge. */
const BAR_CLEARANCE = 1;
/** The clamp's screw, an M3 socket head: its length under the head, its head's diameter and height. */
export const CLAMP_SCREW = 12;
const HEAD_DIAMETER = 5.5;
const HEAD_HEIGHT = 3;
const WHEEL_HEIGHT = 5;
/** How far the jaw stands open below the slat before the screw closes it. */
const JAW_OPEN = 0.5;
/** The jaw's boss around the screw, along Z either side of it. */
const BOSS_HALF = 4;
/** The Pico's parts above its board, the micro-USB socket the tallest. */
const PICO_PARTS = 2.6;

export class VentMount {
  /** The bar's span in Y: under the rim, then over it to its top face. */
  readonly barY: [number, number];
  readonly barTop: number;
  readonly topPosts: [number, number][];
  readonly bottomPosts: [number, number][];
  /** The slat's top and bottom, and the screw's distance from the tray's back. */
  readonly slatTop: number;
  readonly slatBottom: number;
  readonly screwZ: number;

  constructor(
    readonly c: Config,
    readonly faceplate: Faceplate,
  ) {
    const { vent, boards, tray, rim } = c;
    const posts = faceplate.posts;
    const postY = posts[0][1];
    this.topPosts = posts.filter(([, y]) => y === postY);
    this.bottomPosts = posts.filter(([, y]) => y !== postY);
    const picoTop = boards.pico.center[1] + boards.pico.length / 2;
    // Between the Pico and the rim along the top edge, then over the rim.
    this.barY = [picoTop + BAR_CLEARANCE, faceplate.top - rim.width - 0.2];
    this.barTop = faceplate.top - 0.3;
    this.slatTop = this.barTop - vent.legThickness;
    this.slatBottom = this.slatTop - vent.slatThickness;
    this.screwZ = (BAR_THICKNESS + vent.standoff) / 2;

    if (rim.height + c.clearance > BAR_THICKNESS - 1) {
      throw new Error("rim.height leaves the vent clamp's bar too thin over the rim");
    }
    if (vent.legThickness > vent.slatGap - 1) {
      throw new Error("vent.legThickness must leave 1 mm in vent.slatGap: each leg goes between two slats");
    }
    if (Math.min(vent.standoff, vent.feetLength) <= c.converter.height + 1) {
      throw new Error(
        "vent.standoff and vent.feetLength must clear the converter on the tray's back (converter.height)",
      );
    }
    if (vent.wheelDiameter + 1 > vent.standoff - BAR_THICKNESS) {
      throw new Error(
        "vent.wheelDiameter: the thumbwheel doesn't fit between the bar and the vent's face, raise vent.standoff",
      );
    }
    if (this.bossBottom > this.jawTop - vent.legThickness - 2) {
      throw new Error("the clamp's screw (M3x12) is too short for the legs and the slat: thinner legs or slat");
    }
    // The jaw's screw boss reaches down behind the Pico's top end.
    const picoParts = boards.pico.standoffHeight + 1 + PICO_PARTS;
    if (this.bossBottom < picoTop && this.screwZ - BOSS_HALF < picoParts + 1) {
      throw new Error("the vent clamp's jaw runs into the Pico's parts: raise vent.standoff");
    }
    if (tray.screwClearance / 2 + 1 > postY - this.barY[0]) {
      throw new Error("the vent clamp's bar leaves too little around the top tray screws");
    }
  }

  /** The jaw's top face, open below the slat. */
  get jawTop(): number {
    return this.slatBottom - JAW_OPEN;
  }

  /** Where the screw's tip reaches: the bottom of the jaw's boss, half a millimeter past it. */
  get bossBottom(): number {
    return this.barTop - CLAMP_SCREW + 0.5;
  }

  /** The legs' far end, over the slat but short of its back edge. */
  get legEnd(): number {
    return this.c.vent.standoff + this.c.vent.slatDepth - 1;
  }

  /** The bar and the upper leg, in place behind the tray. */
  get bar(): ScadObject {
    const { vent, tray, rim, clearance } = this.c;
    const [y0, y1] = this.barY;
    const [x0, x1] = [this.topPosts[0][0], this.topPosts[1][0]];
    const half = vent.legWidth / 2;
    const headR = tray.postDiameter / 2; // fits the rim's notch around each post
    const body = union(
      box(x0, x1, y0, y1, 0, BAR_THICKNESS),
      box(x0, x1, y1 - 0.01, this.barTop, rim.height + clearance, BAR_THICKNESS), // over the rim: the face it prints on
      ...this.topPosts.map(([x, y]) => cylinder(BAR_THICKNESS, headR, { center: false }).translate([x, y, 0])),
      box(-half, half, this.slatTop, this.barTop, BAR_THICKNESS - 0.01, this.legEnd),
    );
    const screwHole = cylinder(vent.legThickness + 2, tray.screwClearance / 2, { center: false })
      .rotate([-90, 0, 0])
      .translate([0, this.slatTop - 1, this.screwZ]);
    const postHoles = this.topPosts.map(([x, y]) =>
      cylinder(BAR_THICKNESS + 2, tray.screwClearance / 2, { center: false }).translate([x, y, -1]),
    );
    return difference(body, screwHole, ...postHoles);
  }

  /** The jaw: the lower leg under the slat, its back end bearing on the upper leg, its boss for the screw. */
  get jaw(): ScadObject {
    const { vent, screw, clearance } = this.c;
    const half = vent.legWidth / 2;
    const jawTop = this.jawTop;
    const back = BAR_THICKNESS + clearance;
    const body = union(
      box(-half, half, jawTop - vent.legThickness, jawTop, back, this.legEnd),
      box(-half, half, jawTop - 0.01, this.slatTop, back, back + 3), // the heel, against the upper leg
      box(
        -half,
        half,
        this.bossBottom,
        jawTop - vent.legThickness + 0.01,
        this.screwZ - BOSS_HALF,
        this.screwZ + BOSS_HALF,
      ),
    );
    const pilot = cylinder(jawTop - this.bossBottom + 2, screw.m3PilotDiameter / 2, { center: false })
      .rotate([-90, 0, 0])
      .translate([0, this.bossBottom - 1, this.screwZ]);
    return difference(body, pilot);
  }

  /** The thumbwheel, standing: the screw's head pressed into it from below, a hole on top for the hex key. */
  get wheel(): ScadObject {
    const r = this.c.vent.wheelDiameter / 2;
    const notches = Array.from({ length: 12 }, (_, i) => {
      const a = (i * Math.PI) / 6;
      return cylinder(WHEEL_HEIGHT + 2, 1, { center: false }).translate([r * Math.cos(a), r * Math.sin(a), -1]);
    });
    return difference(
      cylinder(WHEEL_HEIGHT, r, { center: false }),
      ...notches,
      cylinder(HEAD_HEIGHT + 0.01, HEAD_DIAMETER / 2, { center: false }).translate([0, 0, -0.01]),
      cylinder(WHEEL_HEIGHT + 2, 1.6, { center: false }).translate([0, 0, -1]),
    );
  }

  /** The wheel in place on the upper leg, its axis along Y. */
  get wheelInPlace(): ScadObject {
    // Its Z becomes the car's Y: standing on the upper leg.
    return this.wheel.rotate([-90, 0, 0]).translate([0, this.barTop, this.screwZ]);
  }

  /** The 2 feet on the bottom posts, against what's under the vent. */
  get feet(): ScadObject {
    const { vent, tray } = this.c;
    return union(
      ...this.bottomPosts.map(([x, y]) =>
        difference(
          cylinder(vent.feetLength, (tray.postDiameter - 0.3) / 2, { center: false }),
          cylinder(vent.feetLength + 2, tray.screwClearance / 2, { center: false }).translate([0, 0, -1]),
        ).translate([x, y, 0]),
      ),
    );
  }

  /** The clamp's parts in place, the wheel on. */
  get clamp(): ScadObject {
    return union(this.bar, this.jaw.color("dimgray"), this.wheelInPlace.color("dimgray"));
  }

  /** Every part as it prints, side by side: the bar on its top face, the jaw on its side, the wheel and the feet standing. */
  get print(): Record<string, ScadObject> {
    const half = this.c.vent.legWidth / 2;
    const [[x0, y0], [x1, y1]] = this.bottomPosts;
    return {
      // The car's +Y face down.
      "vent-bar": this.bar.rotate([-90, 0, 0]).translate([0, 0, this.barTop]),
      // On its side: its X becomes up.
      "vent-jaw": this.jaw.rotate([0, -90, 0]).translate([0, 0, half]),
      "vent-wheel": this.wheel,
      "vent-feet": this.feet.translate([-(x0 + x1) / 2, -(y0 + y1) / 2, 0]),
    };
  }
}
