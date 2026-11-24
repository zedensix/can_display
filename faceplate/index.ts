/**
 * The dash's printable parts: the bar, one long, thin box on the steering
 * column's shroud (bar/bar.ts), the display in landscape and the Pico
 * behind it in the middle (shared/case.ts, its front from shared/front.ts),
 * the encoder and the MCP2515 either side, the converter on the cable; and
 * its base (bar/base.ts), the only part taped to the car, the bar clipped
 * into it.
 *
 * Every dimension is in shared/config.ts and bar/config.ts, overridden by
 * data/measured.json.
 */

import type { ScadObject, ScadSpecialVariables } from "scad-js";
import { Bar } from "./bar/bar";
import { BarBase } from "./bar/base";
import { config } from "./shared/config";
import { Front } from "./shared/front";

export const vars: ScadSpecialVariables = { $fn: 64 }; // smoothness of the holes and posts

export const front = new Front(config);
export const bar = new Bar(config, front);
export const barBase = new BarBase(config, bar);

/** Each part to print, as it lies on the bed. */
export const parts: Record<string, ScadObject> = { ...bar.print, "bar-base": barBase.object };
