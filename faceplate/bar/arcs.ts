// The bar's bottom, hollowed into an arc to sit on the steering column's
// shroud, and a test print to pick how deep.

import type { ScadObject } from "scad-js";
import { difference, polygon, union } from "scad-js";
import type { Config } from "../shared/config";
import { box } from "../shared/geometry";

/**
 * Points along an arc through (0, 0), every mm from -half to half, sag
 * below it at both ends: the bar's bottom.
 */
export function arc(sag: number, half: number): [number, number][] {
  const r = (half * half + sag * sag) / (2 * sag);
  const n = Math.ceil(half);
  return Array.from({ length: 2 * n + 1 }, (_, i): [number, number] => {
    const x = Math.max(-half, Math.min(half, i - n));
    return [x, -(r - Math.sqrt(r * r - x * x))];
  });
}

/**
 * A test print to pick the bottom's arc: a strip from -half to half for
 * each of bar.gaugeSags, its bottom that much deeper at the ends, as many
 * notches on its top as its rank. Flat on the bed.
 */
export function arcGauge(c: Config, half: number): ScadObject {
  const { bar: b } = c;
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
