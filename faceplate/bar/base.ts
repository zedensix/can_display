// The bar's base: stuck on the steering column's shroud with VHB tape, the
// bar clipped into it as printed, nothing added to it. A floor under the
// bar's arc, the whole depth (the tape goes under it); at each end a hook
// up the end wall, its lip over the bar's top past the rounded corner:
// flat underneath, it holds the bar down; a 45 degree top, the bar pushed
// down spreads the hooks. A lip rises across the front and the back of
// the floor, so the bar can't slide out either way. To take the bar
// out, pull one hook outward with a thumb, lift that end, then the other.
//
// The hooks and the floor are one profile, across the bar (x, y), extruded
// front to back, the lips at either end of it: it prints standing on its
// front edge, the hooks bending within their layers, never across them
// (the tabs that broke did). The back lip prints last: 1.5 mm straight up
// the bar, printed in the air, then its inside slopes back at 45 degrees.

import type { ScadObject } from "scad-js";
import { difference, intersection, polygon, square, union } from "scad-js";
import type { Config } from "../shared/config";
import type { Bar } from "./bar";

export class BarBase {
  /** The end walls' outside, along x. */
  readonly endX: number;
  /** The bar's top, and its bottom at the ends. */
  readonly topY: number;
  readonly bottomY: number;

  constructor(
    readonly c: Config,
    readonly bar: Bar,
  ) {
    const { bar: b, enclosure: h, clearance } = c;
    this.endX = bar.right + h.wall;
    this.topY = bar.halfY + h.wall;
    this.bottomY = -this.topY - bar.sag;

    const { display: d, window: w } = c;
    const windowBottom = -(d.activeWidth / 2 + w.margin + w.chamfer);
    if (-this.topY + clearance + b.baseLipFront > windowBottom) {
      throw new Error("bar: the base's front lip covers the display's window: check bar.baseLipFront");
    }
    if (b.baseLip < h.cornerRadius) {
      throw new Error("bar: the base's lips stop on the bar's rounded corners, not its top: check bar.baseLip");
    }
    // A cantilever's strain at its root, spread by its lip and the play.
    const reach = this.topY - this.bottomY;
    if ((1.5 * b.baseThickness * (b.baseLip + clearance)) / reach ** 2 > 0.025) {
      throw new Error("bar: the base's hooks bend too much to last: check bar.baseLip or baseThickness");
    }
  }

  /** One hook's profile on the +x end: up the end wall from its rounded bottom corner, its lip over the top. */
  private get hook(): ScadObject {
    const { bar: b, enclosure: h, clearance } = this.c;
    const t = b.baseThickness;
    const r = h.cornerRadius;
    const face = this.endX + clearance;
    const catchY = this.topY + clearance;
    // The band around the outline, from where its bottom corner starts rounding up to where its top one does.
    const band = difference(this.bar.outline.delta_offset(clearance + t), this.bar.outline.delta_offset(clearance));
    const w = r + clearance + t + 1;
    const h0 = this.topY - r - this.bottomY + 10;
    // (scad-js's square is centered.)
    const corner = square([w, h0]).translate([this.endX - r + w / 2, this.topY - r - h0 / 2, 0]);
    const lipTip = 0.8;
    return union(
      intersection(band, corner),
      square([t, catchY - (this.topY - r) + 0.01]).translate([face + t / 2, (catchY + this.topY - r) / 2, 0]),
      polygon([
        [this.endX - b.baseLip, catchY],
        [face + t, catchY],
        [face + t, catchY + lipTip + b.baseLip + clearance + t],
        [this.endX - b.baseLip, catchY + lipTip],
      ]),
    );
  }

  /** The base, as built: the bar's front at z = 0, the front lip under it on the bed. */
  get object(): ScadObject {
    const { bar: b, clearance } = this.c;
    const t = b.baseThickness;
    const outer = this.bar.outline.delta_offset(clearance + t);
    const floor = difference(intersection(outer, this.bar.underArc), this.bar.outline.delta_offset(clearance));
    const profile = union(floor, this.hook, this.hook.mirror([1, 0, 0]));
    // The floor and everything up to height over the bar's bottom.
    const lip = (height: number) => intersection(outer, this.bar.underArc.translate([0, clearance + height, 0]));
    const front = -t - clearance;
    const back = this.bar.back + clearance;
    // The back lip: straight up to ledge, then 45 degree steps back.
    const ledge = Math.min(1.5, b.baseLipBack);
    const step = 0.5;
    const thick = t + b.baseLipBack - ledge;
    const steps = Math.ceil((b.baseLipBack - ledge) / step);
    return union(
      profile.linear_extrude(back + thick - front).translate([0, 0, front]),
      lip(b.baseLipFront).linear_extrude(t).translate([0, 0, front]),
      lip(ledge).linear_extrude(thick).translate([0, 0, back]),
      ...Array.from({ length: steps }, (_, k) =>
        lip(Math.min(b.baseLipBack, ledge + (k + 1) * step))
          .linear_extrude(thick - (k + 1) * step)
          .translate([0, 0, back + (k + 1) * step]),
      ),
    );
  }
}
