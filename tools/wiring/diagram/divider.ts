// The SO divider, drawn where it's soldered, at the Pico: the MCP2515's SO
// swings to 5V, too much for GP12. A 1kΩ in series on the SO wire, a 2kΩ
// from GP12 (pin 16) to GND (pin 18), stepping over GP13's wire (pin 17)
// with a hop: crossing, not joined.

import { PIN_PITCH, picoPin, type Theme } from "./data";
import type { Wire } from "./route";
import { svgElement, svgText, wireColor } from "./svg";

// The color bands: 1kΩ brown black red gold, 2kΩ red black red gold.
const BANDS: Record<string, string[]> = {
  "1kΩ": ["#7c2d12", "#111827", "#dc2626", "#ca8a04"],
  "2kΩ": ["#dc2626", "#111827", "#dc2626", "#ca8a04"],
};

// A resistor lying along a horizontal wire, centered at cx, cy.
function drawResistorBody(svg: Element, theme: Theme, cx: number, cy: number, value: string) {
  const color = theme.board !== null;
  svgElement(
    "rect",
    {
      x: cx - 18,
      y: cy - 6.5,
      width: 36,
      height: 13,
      rx: 6,
      fill: color ? "#e7d3a8" : theme.background,
      stroke: color ? "#8b6f47" : theme.line,
      "stroke-width": 1.5,
    },
    svg,
  );
  BANDS[value].forEach((band, i) => {
    svgElement(
      "rect",
      { x: cx - 11 + i * 6 + (i === 3 ? 4 : 0), y: cy - 6, width: 3, height: 12, fill: color ? band : theme.line },
      svg,
    );
  });
}

const line = (svg: Element, d: string, stroke: string, width: number) =>
  svgElement("path", { d, fill: "none", stroke, "stroke-width": width, "stroke-linejoin": "round" }, svg);

export function drawDivider(svg: Element, theme: Theme, wires: Wire[]) {
  const so = wires.find((w) => w.label === "SO");
  if (!so) return;
  const [[x0, y], [lane]] = so.points;
  const out = Math.sign(lane - x0);
  const gnd = picoPin(18).y;
  const gp13 = y + PIN_PITCH;
  const width = theme.wireWidth;
  const signal = wireColor(theme, "spi1");
  const ground = wireColor(theme, "gnd");
  const anchor = out < 0 ? "end" : "start";

  // The 1kΩ in series on the SO wire, its value over it.
  const r1 = x0 + out * 82;
  drawResistorBody(svg, theme, r1, y, "1kΩ");
  svgText(svg, theme, r1, y - 10, "1kΩ", { size: 10, weight: 700, anchor: "middle" });

  // The 2kΩ: down from GP12's side of the 1kΩ, over GP13 with a hop, along
  // between GP13 and GND, down onto GND.
  const a = x0 + out * 32;
  const mid = (gp13 + gnd) / 2;
  const b = a + out * 58;
  const r2 = (a + b) / 2;
  // Ringed with the paper's color, so GP13's wire shows a gap under the hop.
  const hop = `M ${a} ${y} V ${gp13 - 7} A 7 7 0 0 ${out < 0 ? 1 : 0} ${a} ${gp13 + 7} V ${mid} H ${r2}`;
  line(svg, hop, theme.background, width + 5);
  line(svg, hop, signal, width);
  line(svg, `M ${r2} ${mid} H ${b} V ${gnd}`, ground, width);
  svgElement("circle", { cx: a, cy: y, r: width + 1.5, fill: signal }, svg);
  svgElement("circle", { cx: b, cy: gnd, r: width + 1.5, fill: ground }, svg);
  drawResistorBody(svg, theme, r2, mid, "2kΩ");
  svgText(svg, theme, b + out * 7, mid + 4, "2kΩ", { size: 10, weight: 700, anchor });
}
