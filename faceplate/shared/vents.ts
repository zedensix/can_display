// A honeycomb like the vent's gauge adapter's grille, for the pod's vents
// and the head's lid. The pod's: the vent's
// air goes in through the can's back end and around the can through the
// face's back, along the boards and behind the display, and out through
// the face, around the display and the encoder. The electronics in the
// duct never sit in still air, and the vent still blows.

import type { ScadObject } from "scad-js";
import { cylinder, union } from "scad-js";

/** A keep-out rectangle, x0..x1 by y0..y1. */
export type Rect = [number, number, number, number];

/**
 * The centers of a honeycomb's cells, each cell a hexagon r across its
 * corners, wall between two: every cell inside the ring rMin..rMax, clear
 * of each rect by wall.
 */
export function honeycomb(
  r: number,
  wall: number,
  rMin: number,
  rMax: number,
  keepOut: Rect[] = [],
): [number, number][] {
  const pitch = r * Math.sqrt(3) + wall; // between 2 cells' centers, across their flats
  const rowPitch = (pitch * Math.sqrt(3)) / 2;
  const cells: [number, number][] = [];
  const rows = Math.ceil(rMax / rowPitch);
  for (let i = -rows; i <= rows; i++) {
    for (let j = -rows - 1; j <= rows + 1; j++) {
      const x = j * pitch + (Math.abs(i) % 2) * (pitch / 2);
      const y = i * rowPitch;
      const d = Math.hypot(x, y);
      const clear = keepOut.every(
        ([x0, x1, y0, y1]) => x + r + wall < x0 || x - r - wall > x1 || y + r + wall < y0 || y - r - wall > y1,
      );
      if (d - r >= rMin && d + r <= rMax && clear) cells.push([x, y]);
    }
  }
  return cells;
}

/** The cells as hexagonal holes along z, z0..z1. */
export function cellHoles(cells: [number, number][], r: number, z0: number, z1: number): ScadObject {
  return union(
    ...cells.map(([x, y]) =>
      cylinder(z1 - z0, r, { center: false, $fn: 6 })
        .rotate([0, 0, 30])
        .translate([x, y, z0]),
    ),
  );
}
