// The Dimensions tab's tray cards: each board on its standoffs, the
// converter, and what the printed parts come out as.

import type { Config } from "../../../faceplate/shared/config";
import { type CardSpec, HOLE, PCB, partDrawing, type Row } from "./drawing";

type Board = Config["boards"]["pico"] | Config["boards"]["mcp2515"];

export function boardCard(
  name: string,
  title: string,
  sub: string,
  board: Board,
  tray: Config["tray"],
  pilot: number,
  fill: string,
): CardSpec {
  const W = board.width,
    L = board.length;
  const k = `boards.${name}`;
  const [hx0, hx1] = [(W - board.holesApartX) / 2, (W + board.holesApartX) / 2];
  const [hy0, hy1] = [(L - board.holesApartY) / 2, (L + board.holesApartY) / 2];
  const holes: [number, number][] = [
    [hx0, hy0],
    [hx1, hy0],
    [hx0, hy1],
    [hx1, hy1],
  ];
  const standoff = pilot + 2 * tray.standoffWall;
  const pico = name === "pico";
  const top = partDrawing("From the back of the tray", [-8, -8, W + 10, L + 6], (g) => {
    g.rect(0, 0, W, L, { ...PCB, fill, key: `${k}.width ${k}.length` });
    for (const [x, y] of holes) {
      g.circle(x, y, standoff / 2, {
        fill: "none",
        stroke: "#ea580c",
        "stroke-width": 0.3,
        "stroke-dasharray": "0.8 0.5",
        key: "standoff",
        hover: true,
      });
      g.circle(x, y, board.holeDiameter / 2, { ...HOLE, key: `${k}.holesApartX ${k}.holesApartY ${k}.holeDiameter` });
      g.circle(x, y, pilot / 2, { fill: "#ea580c", key: "pilot", hover: true });
    }
    if (pico) {
      // Its 2 rows of 20 pins (2.54 pitch, 17.78 apart), through the tray under it.
      for (const y of [L / 2 - 8.89, L / 2 + 8.89]) {
        g.rect(W / 2 - 25.4, y - 1.27, 50.8, 2.54, { fill: "#0a0a0a" });
        for (let i = 0; i < 20; i++)
          g.rect(W / 2 - 25.4 + 1.27 + i * 2.54 - 0.32, y - 0.32, 0.64, 0.64, { fill: "#d4d4d8" });
      }
    }
    if ("usbSocketWidth" in board) {
      const [kx0, ky0, kx1, ky1] = [
        W - board.usbSocketDepth,
        (L - board.usbSocketWidth) / 2,
        W + 1,
        (L + board.usbSocketWidth) / 2,
      ];
      g.rect(kx0, ky0, kx1 - kx0, ky1 - ky0, {
        fill: "#d6d3d1",
        stroke: "#57534e",
        "stroke-width": 0.25,
        key: `${k}.usbSocketWidth ${k}.usbSocketDepth`,
      });
      g.text((kx0 + kx1) / 2, (ky0 + ky1) / 2 - 0.9, "USB", { "font-size": 2.2 });
      g.along(ky0, ky1, W + 6, ky1 - ky0, { key: `${k}.usbSocketWidth` });
      g.across(W - board.usbSocketDepth, W, ky1 + 1.5, board.usbSocketDepth, {
        key: `${k}.usbSocketDepth`,
        hover: true,
      });
    }
    g.across(0, W, L + 3, W, { key: `${k}.width` });
    g.along(0, L, -4, L, { key: `${k}.length` });
    g.across(hx0, hx1, -4, hx1 - hx0, { key: `${k}.holesApartX` });
    g.along(hy0, hy1, hx0 + 3, hy1 - hy0, { key: `${k}.holesApartY`, hover: true });
    g.across(hx0 - board.holeDiameter / 2, hx0 + board.holeDiameter / 2, hy1 + 2, board.holeDiameter, {
      key: `${k}.holeDiameter`,
      hover: true,
    });
    g.across(hx0 - pilot / 2, hx0 + pilot / 2, hy1 + 2, pilot, { key: "pilot", hover: true });
    g.across(hx0 - standoff / 2, hx0 + standoff / 2, hy0 - standoff / 2 - 1.5, standoff, {
      key: "standoff",
      hover: true,
    });
  });
  // Side: the tray at the bottom, the board on its standoffs. The Pico's
  // components face out, its header under it, the pins and their plugs
  // through the tray.
  const sh = board.standoffHeight;
  const below = pico ? 2.5 + 14 - sh : tray.thickness;
  const side = partDrawing("Side, tray at the bottom", [-8, -below - 3, W + 8, sh + 1.6 + 6], (g) => {
    g.rect(-4, -tray.thickness, W + 8, tray.thickness, { fill: "#e7e5e4", stroke: "#a8a29e", "stroke-width": 0.2 });
    for (const x of [hx0, hx1])
      g.rect(x - standoff / 2, 0, standoff, sh, {
        fill: "#e7e5e4",
        stroke: "#57534e",
        "stroke-width": 0.2,
        key: `${k}.standoffHeight standoff`,
      });
    g.rect(0, sh, W, 1.6, { ...PCB, fill, key: `${k}.width` });
    if ("usbSocketDepth" in board) {
      g.rect(1, sh - 2.5, W - 2, 2.5, { fill: "#0a0a0a", opacity: 0.6 });
      g.rect(1, sh - 2.5 - 14, W - 2, 14, { fill: "#52525b", opacity: 0.5 });
      g.text(W / 2, sh - 2.5 - 7.6, "Dupont plugs, through the tray", { "font-size": 2.2, fill: "#fff" });
      g.rect(W - board.usbSocketDepth, sh + 1.6, board.usbSocketDepth + 1, 2.6, {
        fill: "#d6d3d1",
        stroke: "#57534e",
        "stroke-width": 0.2,
        key: `${k}.usbSocketDepth`,
      });
    } else {
      g.rect(W * 0.3, sh + 1.6, 12, 2.5, { fill: "#0a0a0a", opacity: 0.6 });
    }
    g.along(0, sh, -3, sh, { key: `${k}.standoffHeight`, hover: true });
    g.text(W + 4, -tray.thickness / 2 - 0.8, "tray", { "font-size": 2.2 });
  });
  const rows: Row[] = [
    ["Width", W, `${k}.width`],
    ["Length", L, `${k}.length`],
    ["Holes apart, along the width", board.holesApartX, `${k}.holesApartX`],
    ["Holes apart, along the length", board.holesApartY, `${k}.holesApartY`],
    ["The board's mounting holes (3 mm or more: M3 screws, else M2)", board.holeDiameter, `${k}.holeDiameter`],
    [`Pilot hole in the standoffs, for its ${pilot >= 2.5 ? "M3" : "M2"} screws`, pilot, null, "pilot"],
    ["Standoff diameter", standoff, null, "standoff"],
    ["Standoff height", board.standoffHeight, `${k}.standoffHeight`],
  ];
  if ("usbSocketWidth" in board) {
    rows.push(["USB socket width", board.usbSocketWidth, `${k}.usbSocketWidth`]);
    rows.push(["USB socket depth, from the board's edge", board.usbSocketDepth, `${k}.usbSocketDepth`]);
  }
  return { title, sub, drawings: [top, side], rows };
}

export function converterCard(k: Config["converter"]): CardSpec {
  const L = k.length,
    W = k.width,
    total = L + 2 * k.earLength;
  const EAR = "converter.earLength converter.earWidthAtBox converter.earWidthAtEnd";
  const top = partDrawing("From above", [-total / 2 - 8, -W / 2 - 6, total / 2 + 6, W / 2 + 13], (g) => {
    for (const side of [-1, 1]) {
      const box = (side * L) / 2;
      const end = side * (L / 2 + k.earLength);
      g.polygon(
        [
          [box, -k.earWidthAtBox / 2],
          [end, -k.earWidthAtEnd / 2],
          [end, k.earWidthAtEnd / 2],
          [box, k.earWidthAtBox / 2],
        ],
        { fill: "#27272a", key: EAR },
      );
      const slotStart = side > 0 ? end - k.earSlotDepth : end;
      g.rect(slotStart, -k.earSlotWidth / 2, k.earSlotDepth, k.earSlotWidth, {
        fill: "#fffdf8",
        key: "converter.earSlotWidth converter.earSlotDepth",
      });
      // The M3 screw into the tray, at the slot's bottom.
      g.circle(end - side * (k.earSlotDepth - k.earSlotWidth / 2), 0, 1.5, {
        fill: "#a1a1aa",
        stroke: "#3f3f46",
        "stroke-width": 0.2,
      });
    }
    g.rect(-L / 2, -W / 2, L, W, {
      fill: "#18181b",
      stroke: "#000",
      "stroke-width": 0.3,
      key: "converter.length converter.width",
    });
    g.text(0, -0.9, "12V → 5V", { fill: "#fff" });
    g.across(-L / 2, L / 2, W / 2 + 3, L, { key: "converter.length" });
    g.across(-total / 2, total / 2, W / 2 + 9, total, { key: "total" });
    g.along(-W / 2, W / 2, -total / 2 - 4, W, { key: "converter.width" });
    const edge = L / 2 + k.earLength;
    g.across(L / 2, edge, -W / 2 - 2.5, k.earLength, { key: "converter.earLength", hover: true });
    g.along(-k.earWidthAtBox / 2, k.earWidthAtBox / 2, L / 2 + 1.5, k.earWidthAtBox, {
      key: "converter.earWidthAtBox",
      hover: true,
    });
    g.along(-k.earWidthAtEnd / 2, k.earWidthAtEnd / 2, edge + 2.5, k.earWidthAtEnd, {
      key: "converter.earWidthAtEnd",
      hover: true,
    });
    g.along(-k.earSlotWidth / 2, k.earSlotWidth / 2, edge - k.earSlotDepth / 2, k.earSlotWidth, {
      key: "converter.earSlotWidth",
      hover: true,
    });
    g.across(edge - k.earSlotDepth, edge, k.earSlotWidth / 2 + 1.5, k.earSlotDepth, {
      key: "converter.earSlotDepth",
      hover: true,
    });
  });
  const side = partDrawing("Side", [-total / 2 - 8, -3, total / 2 + 3, k.height + 4], (g) => {
    g.rect(-total / 2, 0, k.earLength, 2, { fill: "#27272a", key: EAR });
    g.rect(L / 2, 0, k.earLength, 2, { fill: "#27272a", key: EAR });
    g.rect(-L / 2, 0, L, k.height, { fill: "#18181b", key: "converter.height converter.length" });
    g.along(0, k.height, -total / 2 - 4, k.height, { key: "converter.height", hover: true });
  });
  return {
    title: "12V → 5V converter",
    sub: "Its ears cut off, across the tray's back between the Pico and the MCP2515, between 2 stops, zip-tied through the tray",
    drawings: [top, side],
    rows: [
      ["Box length", L, "converter.length"],
      ["Box width", W, "converter.width"],
      ["Box height", k.height, "converter.height"],
      ["Its middle on the tray, below the display board's top edge", k.fromTop, "converter.fromTop"],
      ["Length with its ears", total, null, "total"],
      ["Ear length", k.earLength, "converter.earLength"],
      ["Ear width at the box", k.earWidthAtBox, "converter.earWidthAtBox"],
      ["Ear width at its end", k.earWidthAtEnd, "converter.earWidthAtEnd"],
      ["Slot width", k.earSlotWidth, "converter.earSlotWidth"],
      ["Slot depth", k.earSlotDepth, "converter.earSlotDepth"],
    ],
  };
}

export type Printed = {
  plateThickness: number;
  outlineWidth: number;
  outlineHeight: number;
  encoderBoard: [number, number];
  depth: number;
  pilots: Record<string, number>;
};

/** What the dimensions make: the outline, the depth, the renders. */
export function printedCard(c: Config, printed: Printed): CardSpec {
  return {
    title: "Printed parts",
    sub: "What these dimensions make (mise run faceplate rebuilds it all)",
    drawings: [],
    rows: [
      ["Outline, across", printed.outlineWidth, null],
      ["Outline, top to bottom", printed.outlineHeight, null],
      ["Faceplate thickness", printed.plateThickness, null],
      ["Gap between faceplate and tray", c.tray.gap, "tray.gap"],
      ["Depth, faceplate front to tray back", printed.depth, null],
      ["Play around every pocket and hole", c.clearance, "clearance"],
    ],
    pictures: ["assembly", "assembly-front", "assembly-side", "faceplate-front", "faceplate-back", "tray-back"],
  };
}
