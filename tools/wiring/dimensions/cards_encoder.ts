// The Dimensions tab's encoder card: the body in its recess, the board on
// its standoffs, the shaft through the plate.

import type { Config } from "../../../faceplate/model/config";
import { type CardSpec, HOLE, partDrawing } from "./drawing";

export function encoderCard(
  e: Config["encoder"],
  screw: Config["screw"],
  plateThickness: number,
  [boardLeft, boardRight]: [number, number],
): CardSpec {
  const [x0, x1] = [-1, 1].map((s) => (s * e.holeSpacing) / 2 + e.holesTowardPins);
  const [y0, y1] = [-e.holesFromShaft, e.holesFromShaft];
  const standoffs: [number, number][] = [
    [x0, y0],
    [x1, y0],
    [x0, y1],
    [x1, y1],
  ];
  const r = e.standoffDiameter / 2;
  const STANDOFF_KEYS = "encoder.holeSpacing encoder.holesFromShaft encoder.holesTowardPins encoder.standoffDiameter";
  const BOARD_KEYS = "encoder.boardLength encoder.boardWidth";
  const PIN_KEYS = "encoder.pinsFromShaft encoder.pinsDepth";
  const pinYs = [-2, -1, 0, 1, 2].map((i) => i * 2.54);
  const top = Math.max(y1 + r + 8, e.belowDisplay + 4, e.boardWidth / 2 + 4);
  const left = Math.min(x0 - r - 7, boardLeft - 5);
  const back = partDrawing(
    "Back, shaft in the middle, the board outlined",
    [left, Math.min(y0 - r - 6, -e.boardWidth / 2 - 6), boardRight + 6, top],
    (g) => {
      g.rect(boardLeft, -e.boardWidth / 2, boardRight - boardLeft, e.boardWidth, {
        fill: "none",
        stroke: "#1e3a8a",
        "stroke-width": 0.4,
        "stroke-dasharray": "1.2 0.8",
        key: BOARD_KEYS,
      });
      for (const y of pinYs)
        g.rect(e.pinsFromShaft - 0.5, y - 0.5, boardRight + 4 - e.pinsFromShaft, 1, { fill: "#d4a017", key: PIN_KEYS });
      g.rect(-e.bodySize / 2, -e.bodySize / 2, e.bodySize, e.bodySize, {
        fill: "#9ca3af",
        stroke: "#374151",
        "stroke-width": 0.3,
        key: "encoder.bodySize",
      });
      g.circle(0, 0, e.bushingDiameter / 2, {
        fill: "none",
        stroke: "#374151",
        "stroke-width": 0.3,
        "stroke-dasharray": "0.8 0.6",
        key: "encoder.bushingDiameter",
      });
      for (const [x, y] of standoffs) {
        g.circle(x, y, r, { fill: "#e7e5e4", stroke: "#57534e", "stroke-width": 0.25, key: STANDOFF_KEYS });
        g.circle(x, y, screw.pilotDiameter / 2, HOLE);
      }
      g.across(x0, x1, y1 + r + 2, x1 - x0, { key: "encoder.holeSpacing" });
      g.along(0, y1, x0 - r - 2, y1, { key: "encoder.holesFromShaft" });
      g.across(0, e.holesTowardPins, y1 + r + 5, e.holesTowardPins, { key: "encoder.holesTowardPins", hover: true });
      g.across(-e.bodySize / 2, e.bodySize / 2, -e.bodySize / 2 - 2, e.bodySize, {
        key: "encoder.bodySize",
        hover: true,
      });
      g.across(-e.bushingDiameter / 2, e.bushingDiameter / 2, 0.5, e.bushingDiameter, {
        key: "encoder.bushingDiameter",
        hover: true,
      });
      g.across(x1 - r, x1 + r, y0 - r - 2, e.standoffDiameter, { key: "encoder.standoffDiameter", hover: true });
      g.line(x0 - r, e.belowDisplay, x1 + r, e.belowDisplay, {
        class: "dim hover-only",
        "stroke-dasharray": "1 0.7",
        key: "encoder.belowDisplay",
      });
      g.text(0, e.belowDisplay + 0.8, "display board's bottom edge", {
        "font-size": 2.2,
        class: "hover-only",
        key: "encoder.belowDisplay",
      });
      g.along(0, e.belowDisplay, x1 + r + 3, e.belowDisplay, { key: "encoder.belowDisplay", hover: true });
      g.across(boardLeft, boardRight, -e.boardWidth / 2 - 3, boardRight - boardLeft, {
        key: "encoder.boardLength",
        hover: true,
      });
      g.along(-e.boardWidth / 2, e.boardWidth / 2, boardRight + 3, e.boardWidth, {
        key: "encoder.boardWidth",
        hover: true,
      });
      g.across(0, e.pinsFromShaft, -e.boardWidth / 2 + 1.5, e.pinsFromShaft, {
        key: "encoder.pinsFromShaft",
        hover: true,
      });
    },
  );
  // Side, cut through the shaft: the front (the driver's side) down.
  const panel = e.panelThickness;
  const recess = e.bodySize / 2 + 0.3;
  const boardZ = panel + e.bodyHeight;
  const shaftEnd = panel - e.bushingLength - 12;
  const wide = Math.max(x1 + r + 6, -boardLeft + 3, boardRight + 3);
  const side = partDrawing(
    "Side, through the shaft (front down)",
    [-wide - 4, shaftEnd - 3, wide + 4, boardZ + 9],
    (g) => {
      g.rect(-wide, 0, wide - recess, plateThickness, { fill: "#e7e5e4", stroke: "#a8a29e", "stroke-width": 0.2 });
      g.rect(recess, 0, wide - recess, plateThickness, { fill: "#e7e5e4", stroke: "#a8a29e", "stroke-width": 0.2 });
      // Header on the knob side: a channel out to the plate's edge, down to a skin at the front, if the pins reach the plate.
      if (e.pinsDepth > 0 && boardZ - e.pinsDepth < plateThickness)
        g.rect(
          e.pinsFromShaft - 1.6,
          boardZ - e.pinsDepth,
          wide - e.pinsFromShaft + 1.6,
          plateThickness - boardZ + e.pinsDepth,
          { fill: "#fffdf8", stroke: "#a8a29e", "stroke-width": 0.2, key: PIN_KEYS },
        );
      // The pins, bent outward along the board: under it on the knob side, else on its back.
      const bend = e.pinsDepth > 0 ? boardZ - 3.8 : boardZ + 1.6 + 3.5;
      g.rect(e.pinsFromShaft - 0.32, Math.min(bend, boardZ - 1), 0.64, Math.abs(bend - boardZ) + 2.6, {
        fill: "#d4a017",
        key: PIN_KEYS,
      });
      g.rect(e.pinsFromShaft - 0.32, bend, boardRight + 4 - e.pinsFromShaft, 0.64, { fill: "#d4a017", key: PIN_KEYS });
      g.rect(-recess, 0, 2 * recess, panel, {
        fill: "#d6d3d1",
        stroke: "#a8a29e",
        "stroke-width": 0.2,
        key: "encoder.panelThickness",
      });
      for (const x of [x0, x1])
        g.rect(x - r, plateThickness, 2 * r, boardZ - plateThickness, {
          fill: "#e7e5e4",
          stroke: "#57534e",
          "stroke-width": 0.2,
          key: STANDOFF_KEYS,
        });
      g.rect(-e.bodySize / 2, panel, e.bodySize, e.bodyHeight, {
        fill: "#9ca3af",
        stroke: "#374151",
        "stroke-width": 0.3,
        key: "encoder.bodySize encoder.bodyHeight",
      });
      g.rect(boardLeft, boardZ, boardRight - boardLeft, 1.6, { fill: "#0f172a", key: BOARD_KEYS });
      g.rect(-e.bushingDiameter / 2, panel - e.bushingLength, e.bushingDiameter, e.bushingLength, {
        fill: "#a1a1aa",
        stroke: "#52525b",
        "stroke-width": 0.2,
        key: "encoder.bushingLength encoder.bushingDiameter",
      });
      g.rect(-3, shaftEnd, 6, panel - e.bushingLength - shaftEnd, {
        fill: "#d4d4d8",
        stroke: "#71717a",
        "stroke-width": 0.2,
      });
      g.rect(-e.bushingDiameter / 2 - 2.5, -e.nutThickness, e.bushingDiameter + 5, e.nutThickness, {
        fill: "#71717a",
        key: "encoder.nutThickness",
      });
      g.text(wide - 3, boardZ + 2.3, "board", { "font-size": 2.2 });
      g.text(-wide + 4, -2.6, "front", { "font-size": 2.2 });
      g.along(panel, boardZ, e.bodySize / 2 + 1.5, e.bodyHeight, { key: "encoder.bodyHeight", hover: true });
      g.along(panel - e.bushingLength, panel, -e.bushingDiameter / 2 - 4, e.bushingLength, {
        key: "encoder.bushingLength",
        hover: true,
      });
      g.along(-e.nutThickness, 0, e.bushingDiameter / 2 + 4, e.nutThickness, {
        key: "encoder.nutThickness",
        hover: true,
      });
      g.along(0, panel, recess + 2, panel, { key: "encoder.panelThickness", hover: true });
      if (e.pinsDepth > 0)
        g.along(boardZ - e.pinsDepth, boardZ, e.pinsFromShaft - 3, e.pinsDepth, {
          key: "encoder.pinsDepth",
          hover: true,
        });
    },
  );
  return {
    title: "KY-040 encoder",
    sub: `The body in its recess, the board on 4 standoffs (2 under its holes, 2 opposite), ${e.pinsDepth > 0 ? "its pins under it in a channel out to the edge" : "its pins on its back"}${e.nutThickness ? ", a nut on the collar from the front" : ": its screws alone hold it, no nut"}`,
    drawings: [back, side],
    rows: [
      ["Collar around the shaft, diameter (threaded or not)", e.bushingDiameter, "encoder.bushingDiameter"],
      ["Collar length, from the encoder's face", e.bushingLength, "encoder.bushingLength"],
      ["Nut and washer (0: no thread, no nut)", e.nutThickness, "encoder.nutThickness"],
      ["Body, square side", e.bodySize, "encoder.bodySize"],
      ["Body height, board to face", e.bodyHeight, "encoder.bodyHeight"],
      ["Board's 2 holes apart, center to center", e.holeSpacing, "encoder.holeSpacing"],
      ["Holes from the shaft's center", e.holesFromShaft, "encoder.holesFromShaft"],
      ["Holes' midpoint from the shaft, toward the pins", e.holesTowardPins, "encoder.holesTowardPins"],
      ["Standoff diameter", e.standoffDiameter, "encoder.standoffDiameter"],
      ["Plate at the encoder", e.panelThickness, "encoder.panelThickness"],
      ["Shaft below the display board", e.belowDisplay, "encoder.belowDisplay"],
      ["Board length, pins' end to the other", e.boardLength, "encoder.boardLength"],
      ["Board width, along the pin row", e.boardWidth, "encoder.boardWidth"],
      ["Pin row from the shaft's center", e.pinsFromShaft, "encoder.pinsFromShaft"],
      [
        "Room under the board for the pins and plugs, knob side (0: header on the back)",
        e.pinsDepth,
        "encoder.pinsDepth",
      ],
    ],
  };
}
