// The pod's sled (pod.ts): a plate across the can's middle, sliding in
// between its 2 pairs of ribs, held between the face's spigot and the
// can's back end. On top the Pico (USB toward the front) then the MCP2515
// (terminal toward the back), on standoffs; under it the converter,
// zip-tied through 4 slots; notches in its edges for the wires from one
// side to the other. It prints flat, the standoffs up.

import type { ScadObject } from "scad-js";
import { cylinder, difference, union } from "scad-js";
import { box } from "../shared/geometry";
import type { Pod } from "./pod";

type Board = { width: number; length: number; holesApartX: number; holesApartY: number; holeDiameter: number };

/** A board's 4 standoffs on the sled's top, centered at z, its width along z: [standoffs, pilot holes]. */
function boardMount(pod: Pod, board: Board, z: number, height: number): [ScadObject, ScadObject] {
  const { screw, tray } = pod.c;
  const p = pod.c.pod;
  const pilot = board.holeDiameter < 3 ? screw.pilotDiameter : screw.m3PilotDiameter;
  const top = p.plateThickness / 2;
  const holes = [-1, 1].flatMap((sz) =>
    [-1, 1].map((sx): [number, number] => [(sx * board.holesApartY) / 2, z + (sz * board.holesApartX) / 2]),
  );
  const along = (r: number, from: number, to: number) => (x: number, hz: number) =>
    cylinder(to - from, r, { center: false })
      .rotate([-90, 0, 0])
      .translate([x, from, hz]);
  return [
    union(...holes.map(([x, hz]) => along(pilot / 2 + tray.standoffWall, top - 0.01, top + height)(x, hz))),
    union(...holes.map(([x, hz]) => along(pilot / 2, -top - 1, top + height + 1)(x, hz))),
  ];
}

/** The sled, as built: the plate across the can at y = 0, from plateFront to plateEnd. */
export function podSled(pod: Pod): ScadObject {
  const { boards, clearance, converter } = pod.c;
  const p = pod.c.pod;
  const half = pod.canIn - 0.5;
  const tongueHalf = pod.bandIn - clearance - 0.1;
  const bandEnd = pod.back + p.canBandLength + 1;
  const t = p.plateThickness / 2;
  const [picoStandoffs, picoHoles] = boardMount(
    pod,
    boards.pico,
    (pod.pico[0] + pod.pico[1]) / 2,
    boards.pico.standoffHeight,
  );
  const [mcpStandoffs, mcpHoles] = boardMount(
    pod,
    boards.mcp2515,
    (pod.mcp[0] + pod.mcp[1]) / 2,
    boards.mcp2515.standoffHeight,
  );
  const slotX = converter.width / 2 + 1;
  const notch = (z0: number) =>
    [-1, 1].map((side) =>
      box(
        side < 0 ? -half - 1 : half - p.notchDepth,
        side < 0 ? -half + p.notchDepth : half + 1,
        -t - 1,
        t + 1,
        z0,
        z0 + p.notchLength,
      ),
    );
  return difference(
    union(
      box(-tongueHalf, tongueHalf, -t, t, pod.plateFront, bandEnd + 0.01),
      box(-half, half, -t, t, bandEnd, pod.plateEnd),
      picoStandoffs,
      mcpStandoffs,
    ),
    picoHoles,
    mcpHoles,
    // The zip ties around the converter, under the plate.
    ...[pod.converter[0] + 10, pod.converter[1] - 10].flatMap((z) =>
      [-1, 1].map((side) =>
        box(
          side < 0 ? -slotX - p.zipSlotWidth : slotX,
          side < 0 ? -slotX : slotX + p.zipSlotWidth,
          -t - 1,
          t + 1,
          z - p.zipSlotLength / 2,
          z + p.zipSlotLength / 2,
        ),
      ),
    ),
    ...notch(bandEnd + 1),
    ...notch(pod.plateEnd - p.notchLength - 2),
  );
}
