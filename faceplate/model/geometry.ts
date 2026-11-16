// Shapes and sizes every part uses.

import type { ScadObject } from "scad-js";
import { cube } from "scad-js";

/** The encoder board's header: 5 pins (CLK, DT, SW, +, GND), in a row across the board. */
export const ENCODER_PINS = 5;
/** Every pin header's pitch. */
export const PITCH = 2.54;

/** A box from x0..x1, y0..y1, z0..z1. */
export function box(x0: number, x1: number, y0: number, y1: number, z0: number, z1: number): ScadObject {
  return cube([x1 - x0, y1 - y0, z1 - z0], { center: false }).translate([x0, y0, z0]);
}
