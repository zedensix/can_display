// The tray's modules as they sit once assembled, for the assembly pictures
// only (never printed): the Pico, the MCP2515 and the converter in the
// tray's frame, roughly, to show which face is which.

import type { ScadObject } from "scad-js";
import { union } from "scad-js";
import type { Config } from "../shared/config";
import { box, DUPONT_LENGTH, HEADER_PINS, HEADER_PLASTIC, PICO_PINS, PICO_ROWS_APART } from "../shared/geometry";
import { display, encoder, pinRow, plugRow, spacer } from "../shared/mockups";
import type { Faceplate } from "./faceplate";

/** The Pico W, components out (its USB socket at the right end), its header under it, the pins and their plugs through the tray. */
function pico(c: Config): ScadObject {
  const { tray } = c;
  const board = c.boards.pico;
  const [x, y] = board.center;
  const base = tray.thickness + board.standoffHeight;
  const face = base + 1;
  const right = x + board.width / 2;
  const left = x - board.width / 2;
  const plastic = base - HEADER_PLASTIC;
  return union(
    box(left, right, y - board.length / 2, y + board.length / 2, base, face).color("green"),
    box(
      right + 1 - board.usbSocketDepth,
      right + 1,
      y - board.usbSocketWidth / 2,
      y + board.usbSocketWidth / 2,
      face,
      face + 2.6,
    ).color("silver"),
    box(x - 3.5, x + 3.5, y - 3.5, y + 3.5, face, face + 1).color("black"), // RP2040
    box(left + 4, left + 14, y - 5, y + 5, face, face + 1.6).color("silver"), // the wireless chip's shield
    ...[-1, 1].flatMap((side) => {
      const rowY = y + (side * PICO_ROWS_APART) / 2;
      return [
        spacer(PICO_PINS, x, rowY, plastic, base),
        pinRow(PICO_PINS, x, rowY, plastic - HEADER_PINS, face + 1.5),
        plugRow(PICO_PINS, x, rowY, plastic - DUPONT_LENGTH, plastic),
      ];
    }),
  );
}

/** The MCP2515 board, components out: the CAN terminal at the top, the 7-pin header at the bottom. */
function mcp2515(c: Config): ScadObject {
  const { tray } = c;
  const board = c.boards.mcp2515;
  const [x, y] = board.center;
  const base = tray.thickness + board.standoffHeight;
  const face = base + 1.6;
  const top = y + board.length / 2;
  const headerY = y - board.length / 2 + 2;
  return union(
    box(x - board.width / 2, x + board.width / 2, y - board.length / 2, top, base, face).color("navy"),
    box(x - 5, x + 5, top - 7.5, top, face, face + 10).color("deepskyblue"), // terminal: H, L
    box(x - 12, x - 2, y - 3, y + 4.5, face, face + 1.5).color("black"), // MCP2515
    box(x + 6, x + 11, y - 1, y + 3, face, face + 1.5).color("black"), // TJA1050
    box(x - 12, x - 1, y - 9, y - 4.5, face, face + 3.5).color("silver"), // 8 MHz crystal
    spacer(7, x, headerY, face, face + 2.5),
    pinRow(7, x, headerY, base - 1.5, face + 8.5),
  );
}

/** The converter, its ears cut off, on the tray's back between its stops, a zip tie around it. */
function converter(c: Config): ScadObject {
  const k = c.converter;
  const y = -k.fromTop;
  const z = c.tray.thickness;
  return union(
    box(-k.length / 2, k.length / 2, y - k.width / 2, y + k.width / 2, z, z + k.height).color("dimgray"),
    box(-2, 2, y - k.width / 2 - 1.5, y + k.width / 2 + 1.5, z + k.height, z + k.height + 1).color("white"),
  );
}

/** Every module in place: the faceplate's in its frame, the tray's in the tray's (move them with it). */
export function mockups(c: Config, faceplate: Faceplate): { faceplate: ScadObject; tray: ScadObject } {
  return {
    faceplate: union(display(c, faceplate), encoder(c, faceplate)),
    tray: union(pico(c), mcp2515(c), converter(c)),
  };
}
