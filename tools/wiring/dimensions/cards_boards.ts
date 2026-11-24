// The Dimensions tab's board cards: the Pico and the MCP2515, each held
// by heat-staked pins through its holes, and what the printed parts come
// out as.

import type { Config } from "../../../faceplate/shared/config";
import { type CardSpec, HOLE, PCB, partDrawing, type Row } from "./drawing";

type Board = Config["boards"]["pico"] | Config["boards"]["mcp2515"];

export function boardCard(name: string, title: string, sub: string, board: Board, pin: number, fill: string): CardSpec {
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
  const top = partDrawing("From above", [-8, -8, W + 10, L + 6], (g) => {
    g.rect(0, 0, W, L, { ...PCB, fill, key: `${k}.width ${k}.length` });
    for (const [x, y] of holes) {
      g.circle(x, y, board.holeDiameter / 2, { ...HOLE, key: `${k}.holesApartX ${k}.holesApartY ${k}.holeDiameter` });
      g.circle(x, y, pin / 2, { fill: "#ea580c", key: "pin", hover: true });
    }
    if ("usbSocketWidth" in board) {
      // Its 2 rows of 20 pins (2.54 pitch, 17.78 apart).
      for (const y of [L / 2 - 8.89, L / 2 + 8.89]) {
        g.rect(W / 2 - 25.4, y - 1.27, 50.8, 2.54, { fill: "#0a0a0a" });
        for (let i = 0; i < 20; i++)
          g.rect(W / 2 - 25.4 + 1.27 + i * 2.54 - 0.32, y - 0.32, 0.64, 0.64, { fill: "#d4d4d8" });
      }
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
  });
  const rows: Row[] = [
    ["Width", W, `${k}.width`],
    ["Length", L, `${k}.length`],
    ["Holes apart, along the width", board.holesApartX, `${k}.holesApartX`],
    ["Holes apart, along the length", board.holesApartY, `${k}.holesApartY`],
    ["The board's mounting holes", board.holeDiameter, `${k}.holeDiameter`],
    ["The heat-staked pin through each", pin, null, "pin"],
  ];
  if ("usbSocketWidth" in board) {
    rows.push(["USB socket width", board.usbSocketWidth, `${k}.usbSocketWidth`]);
    rows.push(["USB socket depth, from the board's edge", board.usbSocketDepth, `${k}.usbSocketDepth`]);
  }
  return { title, sub, drawings: [top], rows };
}

export type Printed = {
  frontThickness: number;
  encoderBoard: [number, number];
  bar: [number, number, number];
};

/** What the dimensions make: the bar's outside, the renders. */
export function printedCard(c: Config, printed: Printed): CardSpec {
  const [length, height, depth] = printed.bar;
  return {
    title: "Printed parts",
    sub: "What these dimensions make (mise run faceplate rebuilds it all)",
    drawings: [],
    rows: [
      ["The bar, end to end", length, null],
      ["The bar, top to bottom at the ends (over its arc)", height, null],
      ["The bar, front to back", depth, null],
      ["Front thickness, at the display", printed.frontThickness, null],
      ["Play around every pocket and hole", c.clearance, "clearance"],
    ],
    pictures: ["bar-front", "bar-inside", "bar-on-base", "bar-shell-back", "bar-lid", "bar-base"],
  };
}
