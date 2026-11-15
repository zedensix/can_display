/**
 * The dash's printable parts: the faceplate holding the ST7789 2" display
 * in portrait and the KY-040 encoder (an EC11) below it, and the tray
 * behind it carrying the Pico W and the MCP2515 CAN board.
 *
 * - Display: the panel shows through a window the size of its lit area,
 *   beveled at the front. From the back, the panel sits in a pocket
 *   against a thin skin of plate, its board in a shallower pocket around
 *   it, flush with the back, held by 4 M2 self-tapping screws.
 * - Encoder: its shaft goes through a hole, its body sits in a square
 *   recess from the back, its board screwed onto standoffs; where the plate
 *   is thin enough for a nut, if its collar is threaded.
 * - Tray: it screws onto 4 posts at the faceplate's corners, leaving room
 *   between the two for the display's and the encoder's connectors. The
 *   boards screw onto standoffs on its back; the wires go through a slot.
 *
 * Every dimension is in config.ts; each part is a class in its own file.
 */

import type { ScadObject, ScadSpecialVariables } from "scad-js";
import { union } from "scad-js";
import { config } from "./config";
import { Faceplate } from "./faceplate";
import { Tray } from "./tray";

export const vars: ScadSpecialVariables = { $fn: 64 }; // smoothness of the holes and posts

export const faceplate = new Faceplate(config);
export const tray = new Tray(config, faceplate);

/** Each part to print, as it lies on the bed. */
export const parts: Record<string, ScadObject> = {
  faceplate: faceplate.object,
  tray: tray.object,
};

/** Everything in place, the boards as colored slabs: for a picture, never printed. */
export const assembly: ScadObject = union(
  faceplate.object.color("gold"),
  union(tray.object.color("silver"), tray.boardMockups).translate([0, 0, faceplate.thickness + config.tray.gap]),
);
