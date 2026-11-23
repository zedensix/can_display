// The Pico on the diagram: its board, its 40 pins named, the used ones lit.

import { PICO, PIN_NAMES, picoPin, type Theme, type WiringView } from "./data";
import { usedPicoPins } from "./route";
import { svgElement, svgText } from "./svg";

export function drawPico(layer: Element, theme: Theme, view: WiringView) {
  // A group, so a click anywhere on the board turns it over.
  const svg = svgElement("g", { "data-module": "pico", class: "module" }, layer);
  const fill = theme.board ? theme.board.pico : "none";
  const stroke = theme.board ? "#0f3d1f" : theme.line;
  svgElement(
    "rect",
    { x: PICO.x, y: PICO.y, width: PICO.w, height: PICO.h, rx: 10, fill, stroke, "stroke-width": 2 },
    svg,
  );
  // USB at the top, the name under it, the wireless module low down.
  svgElement(
    "rect",
    {
      x: PICO.x + 58,
      y: PICO.y - 14,
      width: 44,
      height: 26,
      rx: 4,
      fill: theme.board ? "#a8a29e" : "none",
      stroke: theme.board ? "#57534e" : theme.line,
      "stroke-width": 1.5,
    },
    svg,
  );
  const onBoard = theme.board ? "#fff" : theme.text;
  svgText(svg, theme, PICO.x + 80, PICO.y + 36, "Raspberry Pi", { size: 12, anchor: "middle", color: onBoard });
  svgText(svg, theme, PICO.x + 80, PICO.y + 54, "Pico W(H)", {
    size: 14,
    weight: 700,
    anchor: "middle",
    color: onBoard,
  });
  if (view.faces.pico === "back") {
    // The side without the chips: the header's plastic, the test pads.
    svgText(svg, theme, PICO.x + 80, PICO.y + 70, "seen from below", {
      size: 11,
      weight: 700,
      anchor: "middle",
      color: onBoard,
    });
    return drawPicoPins(svg, theme);
  }
  svgElement(
    "rect",
    {
      x: PICO.x + 44,
      y: 548,
      width: 72,
      height: 52,
      rx: 4,
      fill: theme.board ? "#d6d3d1" : "none",
      stroke: theme.board ? "#57534e" : theme.line,
      "stroke-width": 1.5,
      "stroke-dasharray": theme.board ? "" : "4 3",
    },
    svg,
  );
  svgText(svg, theme, PICO.x + 80, 578, "CYW43", {
    size: 11,
    anchor: "middle",
    color: theme.board ? "#1c1917" : theme.text,
  });
  drawPicoPins(svg, theme);
}

function drawPicoPins(svg: Element, theme: Theme) {
  const used = usedPicoPins();
  for (let n = 1; n <= 40; n++) {
    const p = picoPin(n);
    const isUsed = used.has(n);
    svgElement(
      "circle",
      {
        cx: p.x,
        cy: p.y,
        r: isUsed ? 5 : 3.5,
        fill: isUsed ? theme.pin : "none",
        stroke: theme.pin,
        "stroke-width": 1.5,
      },
      svg,
    );
    const name = PIN_NAMES[n - 1];
    const color = theme.board ? (isUsed ? "#fff" : "rgb(255 255 255 / 0.55)") : isUsed ? theme.text : theme.dim;
    svgText(svg, theme, p.left ? p.x + 10 : p.x - 10, p.y + 4, name, {
      size: 9.5,
      weight: isUsed ? 700 : 400,
      anchor: p.left ? "start" : "end",
      color,
    });
    svgText(svg, theme, p.left ? p.x - 9 : p.x + 9, p.y - 7, String(n), {
      size: 8,
      anchor: p.left ? "end" : "start",
      color: theme.dim,
    });
  }
}
