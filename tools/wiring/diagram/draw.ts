// The wiring diagram: the Pico, its modules and the wires between them,
// in color or as a blueprint, the Pico from above or below.

import { drawCar, drawConverter } from "./car";
import { type Face, MODULES, NET_NAMES, type Style, THEMES, type Theme, type WiringView } from "./data";
import { drawDivider } from "./divider";
import { drawModule } from "./module";
import { drawPico } from "./pico";
import { routeWires, type Wire } from "./route";
import { PAPER, pathThrough, setMirrored, svgElement, svgText, wireColor } from "./svg";

function drawGrid(svg: Element, theme: Theme) {
  const defs = svgElement("defs", {}, svg);
  const small = svgElement(
    "pattern",
    { id: "grid-small", width: 20, height: 20, patternUnits: "userSpaceOnUse" },
    defs,
  );
  svgElement(
    "path",
    { d: "M 20 0 L 0 0 0 20", fill: "none", stroke: "rgb(230 240 255 / 0.12)", "stroke-width": 1 },
    small,
  );
  const big = svgElement("pattern", { id: "grid-big", width: 100, height: 100, patternUnits: "userSpaceOnUse" }, defs);
  svgElement("rect", { width: 100, height: 100, fill: "url(#grid-small)" }, big);
  svgElement(
    "path",
    { d: "M 100 0 L 0 0 0 100", fill: "none", stroke: "rgb(230 240 255 / 0.28)", "stroke-width": 1.2 },
    big,
  );
  svgElement("rect", { ...PAPER, fill: theme.background }, svg);
  svgElement("rect", { ...PAPER, fill: "url(#grid-big)" }, svg);
  // Drawing frame and title block, as on a print.
  svgElement(
    "rect",
    { x: 12, y: 12, width: 1216, height: 796, fill: "none", stroke: theme.line, "stroke-width": 2 },
    svg,
  );
  const tb = { x: 900, y: 690, w: 316, h: 106 };
  svgElement(
    "rect",
    { x: tb.x, y: tb.y, width: tb.w, height: tb.h, fill: "none", stroke: theme.line, "stroke-width": 1.5 },
    svg,
  );
  svgElement("line", { x1: tb.x, y1: tb.y + 40, x2: tb.x + tb.w, y2: tb.y + 40, stroke: theme.line }, svg);
  svgElement("line", { x1: tb.x + 158, y1: tb.y + 40, x2: tb.x + 158, y2: tb.y + tb.h, stroke: theme.line }, svg);
  svgText(svg, theme, tb.x + 12, tb.y + 27, "GT86 GAUGE CLUSTER: WIRING", { size: 15, weight: 700 });
  svgText(svg, theme, tb.x + 12, tb.y + 62, "PICO W + ST7789", { size: 11 });
  svgText(svg, theme, tb.x + 12, tb.y + 82, "KY-040 + MCP2515", { size: 11 });
  svgText(svg, theme, tb.x + 170, tb.y + 62, "SRC: board.h", { size: 11 });
  svgText(svg, theme, tb.x + 170, tb.y + 82, "PINS: GPn", { size: 11 });
}

function drawWire(svg: Element, theme: Theme, w: Wire) {
  const d = pathThrough(w.points);
  const stroke = wireColor(theme, w.net);
  const width = theme.wireWidth + (["v5", "v12", "gnd", "v33"].includes(w.net) && !theme.net ? 1 : 0);
  svgElement(
    "path",
    {
      d,
      fill: "none",
      stroke,
      "stroke-width": width,
      "stroke-linejoin": "round",
      "stroke-linecap": "round",
      class: "wire",
      "data-net": w.net,
    },
    svg,
  );
  // A wider invisible copy, easier to hover and click than the wire itself.
  const hit = svgElement(
    "path",
    { d, fill: "none", stroke: "transparent", "stroke-width": 14, "data-net": w.net },
    svg,
  );
  svgElement("title", {}, hit, w.wire ? `${NET_NAMES[w.net]}: ${w.wire}` : NET_NAMES[w.net]);
}

// A wire's name, on the stretch leaving the Pico, or the one reaching the
// module when that one's too short. Drawn over every wire, ringed with
// the paper's color so a wire passing under it doesn't cross it out.
function drawWireLabel(svg: Element, theme: Theme, w: Wire) {
  if (!w.wire) return;
  const first: [number, number][] = [w.points[0], w.points[1]];
  const last = w.points.slice(-2);
  // The SO wire's Pico end holds its resistor (drawSeriesResistor).
  const [[x0, y0], [x1]] = Math.abs(first[1][0] - first[0][0]) >= 80 && w.label !== "SO" ? first : last;
  const label = svgText(svg, theme, (x0 + x1) / 2, y0 - 6, w.wire, {
    size: 10,
    weight: 600,
    anchor: "middle",
    color: theme.net ? wireColor(theme, w.net) : theme.text,
  });
  for (const [k, v] of Object.entries({
    stroke: theme.background,
    "stroke-width": 4,
    "stroke-linejoin": "round",
    "paint-order": "stroke",
  }))
    label.setAttribute(k, String(v));
}

function drawLegend(svg: Element, theme: Theme) {
  const nets = theme.net;
  if (!nets) return;
  const items = [
    ["v12", "12V car"],
    ["v5", "5V"],
    ["v33", "3.3V"],
    ["gnd", "GND"],
    ["scl", "display SPI"],
    ["clk", "encoder"],
    ["spi1", "MCP2515"],
  ];
  items.forEach(([net, label], i) => {
    const x = 40 + (i % 4) * 120;
    const y = 730 + Math.floor(i / 4) * 26;
    svgElement(
      "line",
      { x1: x, y1: y, x2: x + 30, y2: y, stroke: nets[net], "stroke-width": 5, "stroke-linecap": "round" },
      svg,
    );
    svgText(svg, theme, x + 38, y + 4, label, { size: 12 });
  });
}

export const OTHER_FACE: Record<Face, Face> = { front: "back", back: "front" };

/** Draws the diagram into svg, in style, as view says. */
export function drawDiagram(svg: SVGSVGElement, style: Style, view: WiringView) {
  const theme = THEMES[style];
  svg.replaceChildren();
  if (theme.grid) drawGrid(svg, theme);
  else svgElement("rect", { ...PAPER, fill: theme.background }, svg);
  drawLegend(svg, theme);
  // The Pico seen from below has its pin columns swapped: rather than
  // wires crossing it, the whole layout is mirrored, everything but the
  // legend and the title block. Mirroring also turns every module over,
  // so each is drawn from its other face to show the one asked for.
  const fromBelow = view.faces.pico === "back";
  const layer = fromBelow ? svgElement("g", { transform: "matrix(-1 0 0 1 1240 0)" }, svg) : svg;
  setMirrored(fromBelow);
  const pinFaces = Object.fromEntries(
    Object.entries(view.faces).map(([id, face]) => [id, fromBelow ? OTHER_FACE[face] : face]),
  );
  const wires = routeWires(pinFaces);
  for (const w of wires) drawWire(layer, theme, w);
  const labels = svgElement("g", {}, layer);
  for (const w of wires) drawWireLabel(labels, theme, w);
  drawDivider(labels, theme, wires);
  drawConverter(layer, theme);
  drawPico(layer, theme, view);
  for (const m of MODULES) drawModule(layer, theme, m, view.faces[m.id], pinFaces[m.id]);
  drawCar(layer, theme);
  setMirrored(false);
}
