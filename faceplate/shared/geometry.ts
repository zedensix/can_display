// Shapes and sizes every part uses.

import type { ScadObject } from "scad-js";
import { cube } from "scad-js";

/** The encoder board's header: 5 pins (CLK, DT, SW, +, GND), in a row across the board. */
export const ENCODER_PINS = 5;
/** Every pin header's pitch. */
export const PITCH = 2.54;

/** The Pico's header (Raspberry Pi's drawing): 2 rows of 20 pins, 17.78 apart, centered on the board. */
export const PICO_PINS = 20;
export const PICO_ROWS_APART = 17.78;
/** A pin header's plastic, and its pins past it; a Dupont plug's length. */
export const HEADER_PLASTIC = 2.5;
export const HEADER_PINS = 6;
export const DUPONT_LENGTH = 14;

/** A box from x0..x1, y0..y1, z0..z1. */
export function box(x0: number, x1: number, y0: number, y1: number, z0: number, z1: number): ScadObject {
  return cube([x1 - x0, y1 - y0, z1 - z0], { center: false }).translate([x0, y0, z0]);
}
