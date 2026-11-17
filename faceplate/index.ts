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
 *   recess from the back, its board screwed onto standoffs, its pins in a
 *   pocket; where the plate is thin enough for a nut, if its collar is
 *   threaded.
 * - Tray: it screws onto 4 posts at the faceplate's corners, leaving room
 *   between the two for the display's and the encoder's connectors. The
 *   boards screw onto standoffs on its back.
 *   The Pico sits components out, its pins and their plugs through 2
 *   slots. The 12V-to-5V converter, its ears cut off, lies across the
 *   tray's back between the Pico and the MCP2515, zip-tied.
 *
 * - Pod (pod.ts): the whole dash in the driver's round air vent instead,
 *   the display in landscape and the encoder in a face shaped like the
 *   vent's gauge adapter, the boards on a sled in a can behind it.
 * - Head (head.ts): the smallest box for up high, the display in
 *   landscape and the Pico behind it, everything else on a cable.
 *
 * Every dimension is in shared/config.ts, each model's own in its folder's
 * config.ts; each part is a class in its own file, each model in its folder.
 */

import type { ScadObject, ScadSpecialVariables } from "scad-js";
import { union } from "scad-js";
import { Head } from "./head/head";
import { Faceplate } from "./plate/faceplate";
import { mockups } from "./plate/mockups";
import { Tray } from "./plate/tray";
import { VentMount } from "./plate/ventmount";
import { Pod } from "./pod/pod";
import { config } from "./shared/config";

export const vars: ScadSpecialVariables = { $fn: 64 }; // smoothness of the holes and posts

export const faceplate = new Faceplate(config);
export const tray = new Tray(config, faceplate);
export const ventMount = new VentMount(config, faceplate);
export const pod = new Pod(config, faceplate);
export const head = new Head(config, faceplate);

/** Each part to print, as it lies on the bed, by model: each model's folder gets its STLs. */
export const models: Record<string, Record<string, ScadObject>> = {
  plate: { faceplate: faceplate.object, tray: tray.object, ...ventMount.print },
  pod: pod.print,
  head: head.print,
};

const modules = mockups(config, faceplate);

/** Everything in place with every module modeled, the front at z = 0: for the pictures, never printed. */
export const assembly: ScadObject = union(
  faceplate.object.color("gold"),
  modules.faceplate,
  union(tray.object.color("silver"), modules.tray).translate([0, 0, faceplate.thickness + config.tray.gap]),
);

/** The assembly on its temporary vent mount, for its picture. */
export const onVent: ScadObject = union(
  assembly,
  union(ventMount.clamp, ventMount.feet)
    .color("tomato")
    .translate([0, 0, faceplate.thickness + config.tray.gap + config.tray.thickness]),
);
