// Building SVG by hand: the diagram's layout is computed, not templated.

import type { Theme } from "./data";

const NS = "http://www.w3.org/2000/svg";

// The background (and the blueprint's grid) runs one drawing's size past
// each edge, as far as panning and zooming out can show.
export const PAPER = { x: -1240, y: -820, width: 3720, height: 2460 };

export type Attrs = Record<string, string | number>;

export function svgElement(name: string, attrs: Attrs, parent: Element, text?: string): SVGElement {
  const node = document.createElementNS(NS, name);
  for (const [k, v] of Object.entries(attrs)) node.setAttribute(k, String(v));
  if (text !== undefined) node.textContent = text;
  parent.append(node);
  return node;
}

export type Anchor = "start" | "end" | "middle";

// While the board is drawn from below, the layout is mirrored: each text
// is mirrored back around its own x (so it reads), its anchor swapped (so
// it still runs away from what it labels).
let mirrored = false;
export const MIRRORED_ANCHOR: Record<Anchor, Anchor> = { start: "end", end: "start", middle: "middle" };

export function setMirrored(on: boolean): void {
  mirrored = on;
}

type TextOptions = { size?: number; weight?: number; anchor?: Anchor; color?: string };

export function svgText(svg: Element, theme: Theme, x: number, y: number, value: string, options: TextOptions = {}) {
  const { size = 12, weight = 400, anchor = "start", color = theme.text } = options;
  const attrs: Attrs = {
    x,
    y,
    "font-size": size,
    "font-weight": weight,
    "text-anchor": anchor,
    fill: color,
    "font-family": theme.font,
  };
  if (mirrored)
    Object.assign(attrs, { "text-anchor": MIRRORED_ANCHOR[anchor], transform: `matrix(-1 0 0 1 ${2 * x} 0)` });
  return svgElement("text", attrs, svg, value);
}

export function wireColor(theme: Theme, net: string): string {
  return theme.net ? theme.net[net] : theme.line;
}

/** An SVG path through points. */
export const pathThrough = (points: [number, number][]) =>
  points.map(([x, y], i) => `${i ? "L" : "M"} ${x} ${y}`).join(" ");
