// The encoder board as soldered: its legs wired straight to the Pico.

import type { Theme } from "./data";
import { pathThrough, svgElement, svgText, wireColor } from "./svg";

// The KY-040 seen from its back, as screwed on the dash (header at the
// top), wired on the encoder's own legs: its header's pads are gone. The
// legs' places are read off a photo of the board, in mm from its top
// left corner. Not mirrored with the Pico: it shows the board as you see
// it with the iron in hand.
const ENCODER_LEGS = {
  board: [18.5, 26],
  header: [3.9, 6.44, 8.98, 11.52, 14.06].map((x, i) => ({ x, y: 1.9, label: ["GND", "+", "SW", "DT", "CLK"][i] })),
  shaft: [8.9, 15],
  rivets: [
    [1.6, 6.3],
    [1.6, 19.8],
  ],
  // R2 and R3, A's and B's pull-ups to +: with the + pad gone, they tie A
  // and B together through 20K, and the knob skips every other click.
  // Removed: the Pico's own pull-ups do the job. R1 (the button's) is empty.
  resistors: [
    { label: "R2 remove", x: [9, 11.7], y: 14.6 },
    { label: "R3 remove", x: [9, 11.7], y: 17.9 },
  ],
  r1: { x: 11.4, y: [10.3, 12.3] },
  // The board's own traces tie C to the button's left leg: the right one,
  // next to R1, is the button's signal. Bridging C to it shorts the button.
  legs: [
    { x: 6.4, y: 8, label: "SW", net: "gnd" },
    { x: 11.4, y: 8, label: "SW", net: "sw", to: "GP4 · Pico pin 6", route: [[11.4, 4.6]] },
    { x: 6.4, y: 22.5, label: "A", net: "dt", to: "GP3 · Pico pin 5", route: [[6.4, 25.4]] },
    { x: 8.9, y: 22.5, label: "C", net: "gnd", to: "GND · Pico pin 3", route: [[8.9, 24.4]] },
    { x: 11.4, y: 22.5, label: "B", net: "clk", to: "GP2 · Pico pin 4", route: [[11.4, 23.4]] },
  ],
  // C to the button's left leg, through the board: nothing to solder.
  trace: [
    [6.4, 8],
    [4.6, 9.5],
    [4.6, 20.5],
    [8.9, 22.5],
  ],
};

/** Draws the encoder board as soldered into svg. */
export function drawEncoderLegs(svg: SVGSVGElement, theme: Theme) {
  svg.replaceChildren();
  svgElement("rect", { x: 0, y: 0, width: 1000, height: 540, fill: theme.background }, svg);
  const e = ENCODER_LEGS;
  const S = 16; // px per mm
  const [ox, oy] = [110, 96];
  const at = ([x, y]: [number, number]): [number, number] => [ox + x * S, oy + y * S];
  const labelX = ox + 24 * S;
  const light = theme.board ? "#fff" : theme.text;
  const dark = theme.board ? "#0b1220" : theme.background;
  svgText(svg, theme, 24, 34, "KY-040 encoder, soldered on its legs", { size: 16, weight: 700 });
  svgText(svg, theme, 24, 54, "seen from the back, as on the dash: header at the top, knob behind", {
    size: 12,
    color: theme.dim,
  });

  const [w, h] = e.board;
  svgElement(
    "rect",
    {
      x: ox,
      y: oy,
      width: w * S,
      height: h * S,
      rx: 6,
      fill: theme.board ? theme.board.encoder : "none",
      stroke: theme.board ? "#334155" : theme.line,
      "stroke-width": 2,
    },
    svg,
  );
  // The encoder's body, on the knob side: dashed.
  const [sx, sy] = at(e.shaft as [number, number]);
  svgElement(
    "rect",
    {
      x: sx - 6.25 * S,
      y: sy - 6.25 * S,
      width: 12.5 * S,
      height: 12.5 * S,
      fill: "none",
      stroke: theme.dim,
      "stroke-width": 1.2,
      "stroke-dasharray": "6 4",
    },
    svg,
  );
  svgText(svg, theme, ox - 8, sy - 2, "encoder body", { size: 10.5, anchor: "end", color: theme.dim });
  svgText(svg, theme, ox - 8, sy + 12, "(dashed, knob side)", { size: 10.5, anchor: "end", color: theme.dim });

  for (const [x, y] of e.rivets.map((p) => at(p as [number, number]))) {
    svgElement(
      "circle",
      {
        cx: x,
        cy: y,
        r: 1.8 * S,
        fill: theme.board ? "#f5f5f4" : "none",
        stroke: theme.board ? "#a8a29e" : theme.line,
        "stroke-width": 1.5,
      },
      svg,
    );
  }
  svgText(svg, theme, ox - 8, at(e.rivets[0] as [number, number])[1] - 2.2 * S, "filament rivets", {
    size: 10.5,
    anchor: "end",
    color: theme.dim,
  });

  for (const r of e.resistors) {
    for (const x of r.x) {
      const [px, py] = at([x, r.y]);
      svgElement(
        "rect",
        {
          x: px - 0.6 * S,
          y: py - 0.5 * S,
          width: 1.2 * S,
          height: S,
          fill: theme.board ? "#cbd5e1" : "none",
          stroke: light,
          "stroke-width": 1,
        },
        svg,
      );
    }
    const [lx, ly] = at([(r.x[0] + r.x[1]) / 2, r.y]);
    svgElement(
      "rect",
      {
        x: lx - 0.75 * S,
        y: ly - 0.45 * S,
        width: 1.5 * S,
        height: 0.9 * S,
        fill: "#111827",
        stroke: light,
        "stroke-width": 0.6,
      },
      svg,
    );
    const half = 1.8 * S;
    svgElement(
      "path",
      {
        d: `M ${lx - half} ${ly - 0.7 * S} L ${lx + half} ${ly + 0.7 * S} M ${lx + half} ${ly - 0.7 * S} L ${lx - half} ${ly + 0.7 * S}`,
        stroke: "#dc2626",
        "stroke-width": 2,
      },
      svg,
    );
    svgText(svg, theme, at([5.6, r.y])[0], ly + 4, r.label, { size: 10, anchor: "end", color: light });
  }
  const [r1x, r1y0] = at([e.r1.x, e.r1.y[0]]);
  const r1y1 = at([e.r1.x, e.r1.y[1]])[1];
  svgElement(
    "rect",
    {
      x: r1x - 0.6 * S,
      y: r1y0 - 0.5 * S,
      width: 1.2 * S,
      height: r1y1 - r1y0 + S,
      fill: "none",
      stroke: light,
      "stroke-width": 1,
      "stroke-dasharray": "3 3",
    },
    svg,
  );
  svgText(svg, theme, at([5.6, 0])[0], (r1y0 + r1y1) / 2 + 4, "R1 empty", { size: 10, anchor: "end", color: light });

  // The header's pads: torn off, unused.
  for (const pad of e.header) {
    const [x, y] = at([pad.x, pad.y]);
    svgElement(
      "circle",
      { cx: x, cy: y, r: 0.8 * S, fill: "none", stroke: theme.dim, "stroke-width": 1.2, "stroke-dasharray": "3 2" },
      svg,
    );
    svgElement(
      "path",
      {
        d: `M ${x - 6} ${y - 6} L ${x + 6} ${y + 6} M ${x + 6} ${y - 6} L ${x - 6} ${y + 6}`,
        stroke: "#dc2626",
        "stroke-width": 2,
      },
      svg,
    );
    svgText(svg, theme, x, oy - 8, pad.label, { size: 9.5, anchor: "middle", color: theme.dim });
  }

  // Each wire, ringed with the background so a dark one shows on the board.
  const wire = (points: [number, number][], net: string, dashed = false) => {
    const d = pathThrough(points);
    svgElement(
      "path",
      {
        d,
        fill: "none",
        stroke: dark === theme.background ? theme.background : "#e2e8f0",
        "stroke-width": theme.wireWidth + 3,
        "stroke-linejoin": "round",
        "stroke-linecap": "round",
      },
      svg,
    );
    svgElement(
      "path",
      {
        d,
        fill: "none",
        stroke: wireColor(theme, net),
        "stroke-width": theme.wireWidth,
        "stroke-linejoin": "round",
        "stroke-linecap": "round",
        ...(dashed ? { "stroke-dasharray": "8 5" } : {}),
      },
      svg,
    );
  };
  // The board's trace from C to the button's left leg: thin, it's copper, not a wire.
  const trace = pathThrough(e.trace.map((p) => at(p as [number, number])));
  svgElement(
    "path",
    {
      d: trace,
      fill: "none",
      stroke: theme.board ? "#b45309" : theme.line,
      "stroke-width": 2,
      "stroke-dasharray": "2 3",
    },
    svg,
  );
  const [tx, ty] = at([4.6, 14]);
  svgText(svg, theme, tx + 6, ty + 30, "board trace", { size: 10, color: light });
  svgText(svg, theme, tx + 6, ty + 43, "C to SW", { size: 10, color: light });
  for (const leg of e.legs.filter((l) => l.to)) {
    const points = [[leg.x, leg.y], ...(leg.route ?? [])].map((p) => at(p as [number, number]));
    const y = points[points.length - 1][1];
    wire([...points, [labelX - 10, y]], leg.net, false);
    svgText(svg, theme, labelX, y + 4, leg.to ?? "", {
      size: 12,
      weight: 700,
      color: theme.net && theme.net[leg.net] !== "#1c1917" ? theme.net[leg.net] : theme.text,
    });
  }
  for (const leg of e.legs) {
    const [x, y] = at([leg.x, leg.y]);
    svgElement(
      "circle",
      { cx: x, cy: y, r: 0.85 * S, fill: theme.pin, stroke: theme.board ? "#78350f" : theme.line, "stroke-width": 1.2 },
      svg,
    );
    svgElement("circle", { cx: x, cy: y, r: 0.3 * S, fill: theme.board ? "#1c1917" : theme.background }, svg);
    svgText(svg, theme, x, y - 1.2 * S, leg.label, { size: 11, weight: 700, anchor: "middle", color: light });
  }

  // What to know, beside the drawing.
  const notes = [
    ["No bridge to solder", "one would short the button: stuck pressed"],
    ["Row of 3: A, C, B", "C, the middle one, is the common: GND"],
    ["Row of 2: the button", "the right one to GP4; the board ties the left to C"],
    ["R2 and R3: remove them", "+ is gone: they tie A to B, every other click lost"],
    ["Turns the wrong way?", "swap GP2 and GP3 at the Pico"],
    ["No + wire", "the Pico's pull-ups replace R2 and R3"],
  ];
  notes.forEach(([title, text], i) => {
    svgText(svg, theme, labelX + 230, 150 + i * 46, title, { size: 12, weight: 700 });
    svgText(svg, theme, labelX + 230, 166 + i * 46, text, { size: 11.5, color: theme.dim });
  });
}
