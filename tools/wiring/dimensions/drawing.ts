// The Dimensions tab's drawings: SVGs in millimeters, y up, every shape
// and dimension tagged with the config keys it shows, so hovering a key's
// row highlights them.

import { type Attrs, svgElement } from "../diagram/svg";

export const mm = (value: number) => `${+value.toFixed(2)} mm`;

export const PCB = { fill: "#1e3a8a", stroke: "#0b1220", "stroke-width": 0.3 };
export const HOLE = { fill: "#fffdf8", stroke: "#0b1220", "stroke-width": 0.2 };

/** A shape's attributes, with key (config keys, space separated) and hover (shown only while highlighted). */
export type Tagged = { key?: string; hover?: boolean; [attribute: string]: string | number | boolean | undefined };
type DimensionOptions = { key?: string; hover?: boolean };

export type Pen = {
  rect: (x: number, y: number, w: number, h: number, attrs?: Tagged) => void;
  circle: (x: number, y: number, r: number, attrs?: Tagged) => void;
  polygon: (points: [number, number][], attrs?: Tagged) => void;
  line: (xa: number, ya: number, xb: number, yb: number, attrs?: Tagged) => void;
  text: (x: number, y: number, value: string, attrs?: Tagged) => void;
  across: (xa: number, xb: number, y: number, value: number, opts?: DimensionOptions) => void;
  along: (ya: number, yb: number, x: number, value: number, opts?: DimensionOptions) => void;
};

/** An SVG in millimeters, y up, framing bounds [x0, y0, x1, y1], under its caption. */
export function partDrawing(caption: string, bounds: [number, number, number, number], draw: (pen: Pen) => void) {
  const [x0, y0, x1, y1] = bounds;
  const figure = document.createElement("figure");
  figure.className = "view";
  const title = document.createElement("figcaption");
  title.textContent = caption;
  figure.append(title);
  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("viewBox", `${x0} ${-y1} ${x1 - x0} ${y1 - y0}`);
  svg.setAttribute("class", "part");
  svg.style.width = `${(x1 - x0) * 5}px`; // every drawing at the same scale, 5 px per mm
  figure.append(svg);
  const keyed = ({ key, hover, ...rest }: Tagged = {}): Attrs => {
    const attrs: Attrs = {};
    for (const [name, value] of Object.entries(rest)) {
      if (typeof value === "string" || typeof value === "number") attrs[name] = value;
    }
    if (key) attrs["data-key"] = key;
    if (hover) attrs.class = `${attrs.class ?? ""} hover-only`.trim();
    return attrs;
  };
  const dimension = (
    opts: DimensionOptions,
    build: (group: Element, bar: (xa: number, ya: number, xb: number, yb: number) => void) => void,
  ) => {
    const group = svgElement("g", keyed({ ...opts, class: "dimension" }), svg);
    const bar = (xa: number, ya: number, xb: number, yb: number) =>
      svgElement("line", { x1: xa, y1: -ya, x2: xb, y2: -yb, class: "dim" }, group);
    build(group, bar);
  };
  draw({
    rect: (x, y, w, h, attrs) => svgElement("rect", { x, y: -(y + h), width: w, height: h, ...keyed(attrs) }, svg),
    circle: (x, y, r, attrs) => svgElement("circle", { cx: x, cy: -y, r, ...keyed(attrs) }, svg),
    polygon: (points, attrs) =>
      svgElement("polygon", { points: points.map(([x, y]) => `${x},${-y}`).join(" "), ...keyed(attrs) }, svg),
    line: (xa, ya, xb, yb, attrs) => svgElement("line", { x1: xa, y1: -ya, x2: xb, y2: -yb, ...keyed(attrs) }, svg),
    text: (x, y, value, attrs = {}) =>
      svgElement("text", { x, y: -y, "text-anchor": "middle", ...keyed(attrs) }, svg, value),
    // A horizontal dimension from xa to xb at height y, its value above.
    across: (xa, xb, y, value, opts = {}) =>
      dimension(opts, (group, bar) => {
        bar(xa, y, xb, y);
        bar(xa, y - 1, xa, y + 1);
        bar(xb, y - 1, xb, y + 1);
        svgElement(
          "text",
          { x: (xa + xb) / 2, y: -y - 0.8, "text-anchor": "middle", class: "dim-value" },
          group,
          mm(value),
        );
      }),
    // A vertical dimension from ya to yb at x, its value turned beside it.
    along: (ya, yb, x, value, opts = {}) =>
      dimension(opts, (group, bar) => {
        bar(x, ya, x, yb);
        bar(x - 1, ya, x + 1, ya);
        bar(x - 1, yb, x + 1, yb);
        const ty = -(ya + yb) / 2;
        svgElement(
          "text",
          { x: x - 0.8, y: ty, "text-anchor": "middle", class: "dim-value", transform: `rotate(-90 ${x - 0.8} ${ty})` },
          group,
          mm(value),
        );
      }),
  });
  return figure;
}

/** Highlights what key draws in card's drawings, dimming the rest; null clears. */
export function highlightKey(card: Element, key: string | null) {
  for (const svg of card.querySelectorAll("svg.part")) {
    svg.classList.toggle("focus", Boolean(key));
    for (const element of svg.querySelectorAll<SVGElement>("[data-key]")) {
      element.classList.toggle("hl", Boolean(key) && (element.dataset.key ?? "").split(" ").includes(key ?? ""));
    }
  }
}

// A card's row: its label, the value the parts are built for, the config
// key it edits (null: derived from the others), and what to highlight
// when it has no key of its own.
export type Row = [label: string, value: number, key: string | null, shows?: string];

export type CardSpec = { title: string; sub: string; drawings: HTMLElement[]; rows: Row[]; pictures?: string[] };
