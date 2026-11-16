// One description of the board, its modules and wires (pins as in
// src/firmware/board.h and docs/wiring.md), and the two styles it's drawn in.

export type Face = "front" | "back";
export type Side = "left" | "right";
export type Style = "color" | "blueprint";

// What the drawing shows, kept per browser: each board's face (back is
// the side you wire from: the Pico from below, the modules from their
// pins' side), and how the car data comes in (Data source in the dash's settings).
export type WiringView = { faces: Record<string, Face>; source: "ble" | "wired" };

export type ModulePin = { label: string; pico?: number; net?: string; wire?: string; note?: string };
export type EdgePin = { label: string; note: string; net: string };
export type Module = {
  id: string;
  kind: string;
  name: string;
  sub: string;
  wired?: boolean;
  x: number;
  y: number;
  w: number;
  h: number;
  side: Side;
  pinTop: number;
  pins: ModulePin[];
  edgePins?: EdgePin[];
};

export type Theme = {
  background: string;
  text: string;
  dim: string;
  font: string;
  net: Record<string, string> | null;
  line: string;
  wireWidth: number;
  board: Record<string, string> | null;
  pin: string;
  grid: boolean;
};

// The board, taller than its pin rows: its name goes under the USB port.
export const PICO = { x: 520, y: 112, w: 160, h: 528 };
export const PIN_TOP = 190;
export const PIN_PITCH = 22;
export const PIN_NAMES = [
  "GP0",
  "GP1",
  "GND",
  "GP2",
  "GP3",
  "GP4",
  "GP5",
  "GND",
  "GP6",
  "GP7",
  "GP8",
  "GP9",
  "GND",
  "GP10",
  "GP11",
  "GP12",
  "GP13",
  "GND",
  "GP14",
  "GP15",
  "GP16",
  "GP17",
  "GND",
  "GP18",
  "GP19",
  "GP20",
  "GP21",
  "GND",
  "GP22",
  "RUN",
  "GP26",
  "GP27",
  "AGND",
  "GP28",
  "ADC_VREF",
  "3V3(OUT)",
  "3V3_EN",
  "GND",
  "VSYS",
  "VBUS",
];

// picoPin is where physical pin n (1-40) sits: 1-20 down the left edge,
// 21-40 up the right one, pin 40 next to the USB port.
export function picoPin(n: number) {
  const left = n <= 20;
  return {
    x: left ? PICO.x : PICO.x + PICO.w,
    y: PIN_TOP + (left ? n - 1 : 40 - n) * PIN_PITCH,
    left,
  };
}

// Modules, in the layout with the Pico seen from above. pins: the header
// in its real order, top to bottom along the edge facing the Pico, with
// the module seen from its front; seen from the back, it's reversed.
// pinTop sits half a pitch off the Pico's rows: no module pin shares a
// Pico pin's height, so a wire's stretch along to its module never runs
// over another's stretch leaving the Pico.
// Each wired pin names the Pico's physical pin it goes to (board.h,
// docs/wiring.md). edgePins sit on the far edge, labeled instead of wired.
export const MODULES: Module[] = [
  {
    id: "display",
    kind: "display",
    name: 'ST7789 2.0" 240x320',
    sub: "SPI0 display",
    x: 900,
    y: 392,
    w: 210,
    h: 228,
    side: "right",
    pinTop: 443,
    // Front: screen up, header at the top, GND VDD SCL SDA RES DC CS BL from the left.
    pins: [
      { label: "BL", pico: 29, net: "bl", wire: "GP22" },
      { label: "CS", pico: 22, net: "cs", wire: "GP17" },
      { label: "DC", pico: 26, net: "dc", wire: "GP20" },
      { label: "RES", pico: 27, net: "res", wire: "GP21" },
      { label: "SDA", pico: 25, net: "sda", wire: "GP19 MOSI" },
      { label: "SCL", pico: 24, net: "scl", wire: "GP18 SCK" },
      { label: "VDD", pico: 36, net: "v33", wire: "3V3" },
      { label: "GND", pico: 23, net: "gnd" },
    ],
  },
  {
    id: "encoder",
    kind: "encoder",
    name: "EC11 encoder (KY-040)",
    sub: "wired on its legs",
    x: 110,
    y: 176,
    w: 190,
    h: 150,
    side: "left",
    pinTop: 223,
    // Soldered on the encoder's own legs, not the board's header: its row
    // of 3 (A, C in the middle, B) and of 2 (the button). The Pico's internal pull-ups
    // replace the board's, so no power goes to it. The board's traces
    // tie C to one button leg: one GND wire for both. A on GP3 and B on
    // GP2: the way that turns right on this car. R2 and R3 are removed. Listed in the Pico's order: no
    // header to follow, so no wire crosses another.
    pins: [
      { label: "C + SW", pico: 3, net: "gnd" },
      { label: "B", pico: 4, net: "clk", wire: "GP2 CLK" },
      { label: "A", pico: 5, net: "dt", wire: "GP3 DT" },
      { label: "SW", pico: 6, net: "sw", wire: "GP4" },
    ],
  },
  {
    id: "mcp",
    kind: "mcp",
    name: "MCP2515 + TJA1050",
    sub: "wired CAN",
    wired: true,
    x: 160,
    y: 420,
    w: 140,
    h: 200,
    side: "left",
    pinTop: 465,
    // Front: chips up, header at the bottom, INT SCK SI SO CS GND VCC from the left.
    pins: [
      { label: "VCC", pico: 40, net: "v5", wire: "5V" },
      { label: "GND", pico: 18, net: "gnd" },
      { label: "CS", pico: 17, net: "spi1", wire: "GP13" },
      { label: "SO", pico: 16, net: "spi1", wire: "GP12 ⚠ divider" },
      { label: "SI", pico: 15, net: "spi1", wire: "GP11" },
      { label: "SCK", pico: 14, net: "spi1", wire: "GP10" },
      { label: "INT", note: "unused" },
    ],
    // The screw terminal, on the far edge: CAN-L above CAN-H seen from the front.
    edgePins: [
      { label: "CAN-L", note: "OBD 14 · green/white", net: "spi1" },
      { label: "CAN-H", note: "OBD 6 · green", net: "spi1" },
    ],
  },
];
export const CONVERTER = { x: 900, y: 104, w: 180, h: 84 };

// What each net carries, shown when hovering a wire.
export const NET_NAMES: Record<string, string> = {
  v12: "12V from the car",
  v5: "5V: the converter, through the Pico's USB socket",
  v33: "3.3V from 3V3(OUT)",
  gnd: "GND",
  bl: "display backlight",
  res: "display reset",
  dc: "display data/command",
  sda: "display SPI data (MOSI)",
  scl: "display SPI clock",
  cs: "display chip select",
  clk: "encoder CLK",
  dt: "encoder DT",
  sw: "encoder button",
  spi1: "MCP2515 on SPI1",
};

export const THEMES: Record<Style, Theme> = {
  color: {
    background: "#f4f1ea",
    text: "#1c1917",
    dim: "#78716c",
    font: "system-ui, sans-serif",
    line: "#1c1917",
    net: {
      v12: "#991b1b",
      v5: "#ef4444",
      v33: "#f59e0b",
      gnd: "#1c1917",
      bl: "#eab308",
      res: "#f97316",
      dc: "#ec4899",
      sda: "#8b5cf6",
      scl: "#3b82f6",
      cs: "#10b981",
      clk: "#06b6d4",
      dt: "#0d9488",
      sw: "#a855f7",
      spi1: "#64748b",
    },
    wireWidth: 4,
    board: { pico: "#1f7a3c", display: "#1e3a8a", encoder: "#0f172a", mcp: "#1e40af", dongle: "#18181b" },
    pin: "#d4a017",
    grid: false,
  },
  blueprint: {
    background: "#123d73",
    text: "#e6f0ff",
    dim: "#9dbbe6",
    font: "ui-monospace, Menlo, monospace",
    net: null, // every wire in the line color
    line: "#e6f0ff",
    wireWidth: 2,
    board: null, // outlines only
    pin: "#e6f0ff",
    grid: true,
  },
};
