// The power and the car: the 12V to 5V converter, the fuse tap, the OBD port.

import { CONVERTER, NET_NAMES, PICO, type Theme, type WiringView } from "./data";
import { svgElement, svgText, wireColor } from "./svg";

// The 12V to 5V converter: 2 wires to the car's 12V, its micro-USB plug
// into the Pico's socket.
export function drawConverter(svg: Element, theme: Theme) {
  const c = CONVERTER;
  const stroke = theme.board ? "#000" : theme.line;
  const usbTop = PICO.y - 14;
  const cable = `M ${c.x} ${c.y + 30} C ${c.x - 160} ${c.y + 30}, ${PICO.x + 80} ${usbTop - 70}, ${PICO.x + 80} ${usbTop - 12}`;
  svgElement(
    "path",
    {
      d: cable,
      fill: "none",
      stroke: wireColor(theme, "v5"),
      "stroke-width": theme.wireWidth + 2,
      "stroke-linecap": "round",
      class: "wire",
      "data-net": "v5",
    },
    svg,
  );
  const hit = svgElement(
    "path",
    { d: cable, fill: "none", stroke: "transparent", "stroke-width": 14, "data-net": "v5" },
    svg,
  );
  svgElement("title", {}, hit, `${NET_NAMES.v5}: micro-USB plug`);
  svgElement(
    "rect",
    {
      x: PICO.x + 70,
      y: usbTop - 14,
      width: 20,
      height: 14,
      rx: 2,
      fill: theme.board ? "#1c1917" : "none",
      stroke,
      "stroke-width": 1.5,
    },
    svg,
  );
  svgText(svg, theme, c.x - 10, c.y + 50, "micro-USB plug to the Pico", {
    size: 10,
    weight: 600,
    anchor: "end",
    color: theme.net ? wireColor(theme, "v5") : theme.text,
  });
  for (const [net, y, label] of [
    ["v12", c.y + 30, "IN+ (red)"],
    ["gnd", c.y + 54, "IN− (black)"],
  ] as const) {
    svgElement(
      "path",
      {
        d: `M ${c.x + c.w} ${y} L 1150 ${y}`,
        fill: "none",
        stroke: wireColor(theme, net),
        "stroke-width": theme.wireWidth,
        class: "wire",
        "data-net": net,
      },
      svg,
    );
    svgText(svg, theme, c.x + c.w + 8, y - 6, label, {
      size: 10,
      weight: 600,
      color: theme.net ? wireColor(theme, net) : theme.text,
    });
  }
  svgElement(
    "rect",
    {
      x: c.x,
      y: c.y,
      width: c.w,
      height: c.h,
      rx: 8,
      fill: theme.board ? "#18181b" : "none",
      stroke,
      "stroke-width": 2,
    },
    svg,
  );
  svgText(svg, theme, c.x + c.w / 2, c.y + 38, "12V → 5V", {
    size: 13,
    weight: 700,
    anchor: "middle",
    color: theme.board ? "#fff" : theme.text,
  });
  svgText(svg, theme, c.x + c.w / 2, c.y + 56, "3A, potted", {
    size: 10,
    anchor: "middle",
    color: theme.board ? "#d6d3d1" : theme.dim,
  });
  svgText(svg, theme, c.x + c.w / 2, c.y + c.h + 18, "DC/DC converter", { size: 13, weight: 700, anchor: "middle" });
  svgText(svg, theme, c.x + c.w / 2, c.y + c.h + 34, "on the tray, ears cut off", {
    size: 11,
    anchor: "middle",
    color: theme.dim,
  });
}

export function drawCar(svg: Element, theme: Theme, view: WiringView) {
  const stroke = theme.board ? "#1c1917" : theme.line;
  const c = CONVERTER;
  svgElement(
    "rect",
    {
      x: 1150,
      y: c.y + 14,
      width: 70,
      height: 56,
      rx: 6,
      fill: theme.board ? "#e7e5e4" : "none",
      stroke,
      "stroke-width": 1.5,
    },
    svg,
  );
  svgText(svg, theme, 1185, c.y + 34, "12V ACC", { size: 10, weight: 700, anchor: "middle" });
  svgText(svg, theme, 1185, c.y + 48, "fuse tap", { size: 9, anchor: "middle", color: theme.dim });
  svgText(svg, theme, 1185, c.y + 62, "+ chassis", { size: 9, anchor: "middle", color: theme.dim });
  // The OBD port: the BLE dongle in it, or the MCP2515's CAN wires (labeled on the module).
  if (view.source === "ble") {
    svgElement(
      "rect",
      {
        x: 540,
        y: 694,
        width: 120,
        height: 56,
        rx: 8,
        fill: theme.board ? theme.board.dongle : "none",
        stroke,
        "stroke-width": 2,
      },
      svg,
    );
    svgText(svg, theme, 600, 720, "Vgate iCar", {
      size: 12,
      weight: 700,
      anchor: "middle",
      color: theme.board ? "#fff" : theme.text,
    });
    svgText(svg, theme, 600, 736, "Pro 2S", {
      size: 11,
      anchor: "middle",
      color: theme.board ? "#d6d3d1" : theme.text,
    });
    const link = theme.board ? "#2563eb" : theme.line;
    for (const r of [16, 30, 44]) {
      svgElement(
        "path",
        {
          d: `M ${600 - r} ${670 - r / 3} Q 600 ${670 + r / 2} ${600 + r} ${670 - r / 3}`,
          fill: "none",
          stroke: link,
          "stroke-width": 2,
          "stroke-dasharray": "5 4",
        },
        svg,
      );
    }
    svgText(svg, theme, 660, 664, "BLE", { size: 12, weight: 700, color: link });
  }
  svgElement(
    "rect",
    {
      x: 560,
      y: 750,
      width: 80,
      height: 16,
      rx: 3,
      fill: theme.board ? "#57534e" : "none",
      stroke,
      "stroke-width": 1.5,
    },
    svg,
  );
  const port =
    view.source === "ble"
      ? "OBD-II port (BLE, nothing to wire)"
      : "OBD-II port: CAN-H pin 6 (green), CAN-L pin 14 (green/white)";
  svgText(svg, theme, 600, 786, port, { size: 11, anchor: "middle", color: theme.dim });
}
