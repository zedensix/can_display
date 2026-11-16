// A module on the diagram: its board, a sketch of the face shown, its
// header's pins, and the far edge's labeled instead of wired.

import type { Face, Module, Theme } from "./data";
import { modulePinY } from "./route";
import { type Anchor, MIRRORED_ANCHOR, svgElement, svgText, wireColor } from "./svg";

export function drawModule(svg: Element, theme: Theme, m: Module, face: Face, pinFace: Face) {
  // A group, so a click anywhere on the module turns it over.
  const group = svgElement("g", { "data-module": m.id, class: "module" }, svg);
  const fill = theme.board ? theme.board[m.kind] : "none";
  const stroke = theme.board ? "#0b1220" : theme.line;
  svgElement("rect", { x: m.x, y: m.y, width: m.w, height: m.h, rx: 8, fill, stroke, "stroke-width": 2 }, group);
  const light = theme.board ? "#fff" : theme.text;
  // A sketch of the face you're looking at: the front's screen, knob or
  // chip, or the back's bare board.
  const cx = m.side === "left" ? m.x + m.w * 0.42 : m.x + m.w * 0.6;
  const cy = m.y + m.h / 2;
  if (face === "back") {
    svgText(group, theme, cx, cy + 4, "back", {
      size: 12,
      weight: 700,
      anchor: "middle",
      color: theme.board ? "rgb(255 255 255 / 0.6)" : theme.dim,
    });
  } else if (m.kind === "display") {
    svgElement(
      "rect",
      {
        x: cx - 61,
        y: cy - 80,
        width: 122,
        height: 160,
        rx: 3,
        fill: theme.board ? "#020617" : "none",
        stroke: theme.board ? "#334155" : theme.line,
        "stroke-width": 1.5,
      },
      group,
    );
    svgText(group, theme, cx, cy + 4, "240×320", {
      size: 11,
      anchor: "middle",
      color: theme.board ? "#ff5a1e" : theme.text,
    });
  } else if (m.kind === "encoder") {
    svgElement(
      "circle",
      { cx, cy, r: 38, fill: theme.board ? "#374151" : "none", stroke: light, "stroke-width": 1.5 },
      group,
    );
    svgElement(
      "circle",
      { cx, cy, r: 14, fill: theme.board ? "#9ca3af" : "none", stroke: light, "stroke-width": 1.5 },
      group,
    );
  } else if (m.kind === "mcp") {
    svgElement(
      "rect",
      {
        x: cx - 22,
        y: cy - 18,
        width: 44,
        height: 36,
        rx: 3,
        fill: theme.board ? "#0f172a" : "none",
        stroke: light,
        "stroke-width": 1.2,
      },
      group,
    );
  }
  svgText(group, theme, m.x + m.w / 2, m.y + m.h + 18, m.name, { size: 13, weight: 700, anchor: "middle" });
  svgText(group, theme, m.x + m.w / 2, m.y + m.h + 34, `${m.sub} · seen from the ${face}`, {
    size: 11,
    anchor: "middle",
    color: theme.dim,
  });
  const headerX = m.side === "left" ? m.x + m.w : m.x;
  const inward = m.side === "left" ? -10 : 10;
  const inwardAnchor: Anchor = m.side === "left" ? "end" : "start";
  m.pins.forEach((pin, index) => {
    const y = modulePinY(m, index, pinFace);
    svgElement(
      "circle",
      { cx: headerX, cy: y, r: 4.5, fill: pin.pico ? theme.pin : "none", stroke: theme.board ? "#78350f" : theme.line },
      group,
    );
    svgText(group, theme, headerX + inward, y + 4, pin.label, {
      size: 10.5,
      weight: 700,
      anchor: inwardAnchor,
      color: light,
    });
    if (pin.note)
      svgText(group, theme, headerX - inward, y + 4, pin.note, {
        size: 10,
        anchor: MIRRORED_ANCHOR[inwardAnchor],
        color: theme.dim,
      });
  });
  // The far edge's pins, labeled with where they go rather than wired across the drawing.
  const farX = m.side === "left" ? m.x : m.x + m.w;
  (m.edgePins ?? []).forEach((pin, index) => {
    const order = pinFace === "front" ? index : (m.edgePins ?? []).length - 1 - index;
    const y = m.y + m.h / 2 - 18 + order * 36;
    svgElement(
      "circle",
      { cx: farX, cy: y, r: 4.5, fill: theme.pin, stroke: theme.board ? "#78350f" : theme.line },
      group,
    );
    svgText(group, theme, farX - inward, y + 4, pin.label, {
      size: 10.5,
      weight: 700,
      anchor: MIRRORED_ANCHOR[inwardAnchor],
      color: light,
    });
    const out = m.side === "left" ? -22 : 22;
    svgElement(
      "line",
      {
        x1: farX,
        y1: y,
        x2: farX + out,
        y2: y,
        stroke: wireColor(theme, pin.net),
        "stroke-width": theme.wireWidth,
        "stroke-dasharray": "6 4",
      },
      group,
    );
    svgText(group, theme, farX + out * 1.2, y + 4, pin.note, { size: 10.5, anchor: inwardAnchor, color: theme.dim });
  });
}
