// Where each wire runs, from the Pico's pin to its module's.

import {
  type Face,
  MODULES,
  type Module,
  type ModulePin,
  PICO,
  PIN_PITCH,
  picoPin,
  type Side,
  type WiringView,
} from "./data";

export type Wire = ModulePin & {
  net: string;
  from: ReturnType<typeof picoPin>;
  to: [number, number];
  moduleSide: Side;
  pinSide: Side;
  lane: number;
  farLane: number;
  points: [number, number][];
};

/** The modules drawn: the MCP2515 only with the wired data source. */
export const shownModules = (view: WiringView) => MODULES.filter((m) => !m.wired || view.source === "wired");

/** The y of a module's pin: its place in the header, reversed for the back. */
export function modulePinY(m: Module, index: number, face: Face): number {
  const order = face === "front" ? index : m.pins.length - 1 - index;
  return m.pinTop + order * PIN_PITCH;
}

/** The Pico's pins something is wired to, in the current mode. */
export function usedPicoPins(view: WiringView): Set<number> {
  return new Set(shownModules(view).flatMap((m) => m.pins.flatMap((pin) => (pin.pico ? [pin.pico] : []))));
}

// Wires from the Pico to the modules, routed in the channel between the
// two: along from the Pico's pin to a lane, up or down the lane, along
// to the module's pin. A module on the Pico's other side goes over the
// top. Each wire gets a lane of its own, so none overlaps another.
export function routeWires(view: WiringView, faces: Record<string, Face>): Wire[] {
  const wires: Wire[] = shownModules(view).flatMap((m) =>
    m.pins.flatMap((pin, index) => {
      if (!pin.pico) return [];
      const from = picoPin(pin.pico);
      const toX = m.side === "left" ? m.x + m.w : m.x;
      return [
        {
          ...pin,
          net: pin.net ?? "gnd",
          from,
          to: [toX, modulePinY(m, index, faces[m.id])] as [number, number],
          moduleSide: m.side,
          pinSide: from.left ? "left" : ("right" as Side),
          lane: 0,
          farLane: 0,
          points: [],
        },
      ];
    }),
  );
  const channels = { left: [PICO.x - 24, 300 + 24], right: [PICO.x + PICO.w + 24, 900 - 24] };
  type Lane = { wire: Wire; crossing: boolean };
  const lanes: Record<Side, Lane[]> = { left: [], right: [] };
  // Each wire asks for a lane on the Pico's side, and one on the module's when it crosses over.
  for (const w of wires) {
    if (w.from.y !== w.to[1] || w.pinSide !== w.moduleSide) lanes[w.pinSide].push({ wire: w, crossing: false });
    if (w.pinSide !== w.moduleSide) lanes[w.moduleSide].push({ wire: w, crossing: true });
  }
  for (const side of ["left", "right"] as Side[]) {
    const [near, far] = channels[side];
    const list = lanes[side].sort(
      (a, b) =>
        (a.crossing ? -1 : 0) - (b.crossing ? -1 : 0) || (a.crossing || b.crossing ? 0 : a.wire.from.y - b.wire.from.y),
    );
    list.forEach((entry, i) => {
      const x = near + ((far - near) * (i + 0.5)) / list.length;
      if (entry.crossing) entry.wire.farLane = x;
      else entry.wire.lane = x;
    });
  }
  let over = 0;
  for (const w of wires) {
    const [x0, y0] = [w.from.x, w.from.y];
    const [x1, y1] = w.to;
    if (w.pinSide !== w.moduleSide) {
      const top = 84 - 12 * over++;
      w.points = [
        [x0, y0],
        [w.lane, y0],
        [w.lane, top],
        [w.farLane, top],
        [w.farLane, y1],
        [x1, y1],
      ];
    } else if (y0 === y1) {
      w.points = [
        [x0, y0],
        [x1, y1],
      ];
    } else {
      w.points = [
        [x0, y0],
        [w.lane, y0],
        [w.lane, y1],
        [x1, y1],
      ];
    }
  }
  return wires;
}
