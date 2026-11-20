// The head: the display in landscape with the Pico W right behind it, in
// the smallest box they go in, for up high (the meter hood's top). The
// rest stays out of sight on a cable: the MCP2515 and the converter at
// the OBD port, the encoder wherever the hand falls. The display's SPI
// (62.5 MHz) won't go down a long cable, the MCP2515's (1 MHz) and the
// encoder will: so the Pico sits behind the display.
//
// - The shell: the front with the display's window and pockets (the
//   faceplate's, turned to landscape), 4 pins through the display board's
//   holes, melted into heads with the soldering iron; walls just around the
//   board, nothing standing into the room (the board goes in from the back,
//   as wide as the room); a notch in the end by the Pico's USB socket
//   for the plug, whichever way up the Pico lies, 2 slots for the lid's
//   hooks; its bottom takes 3M VHB tape, no wedge: the hood's slope
//   already leans it back.
// - The lid lies on the walls' back edge all round, 2 snap tabs hanging
//   along the top and bottom walls, their hooks clicking into the slots; a
//   small screwdriver through a slot pushes its hook back in. The Pico
//   stands on it, either way up, on 4 tall standoffs, their pins through
//   its holes melted into heads, each cut flat inside for the USB socket.
//   Vents around them, and the cable's slot, out the back.
//
// Seen from the back like the faceplate: x across, y up, the front at
// z = 0, the display board centered. Both print with no supports: the
// shell front down (the window's bevel, the slots' short bridges), the lid
// outside down (the hooks' 1.8 mm ledges, short enough to print in the air).

import type { ScadObject } from "scad-js";
import { difference, union } from "scad-js";
import type { Faceplate } from "../plate/faceplate";
import { Case } from "../shared/case";
import type { Config } from "../shared/config";
import { box } from "../shared/geometry";

export class Head extends Case {
  /** The cable's slot through the lid, its middle. */
  readonly cable: [number, number];

  constructor(c: Config, faceplate: Faceplate) {
    super(c, faceplate);
    const { head: h, boards } = c;
    // Under the Pico, toward the hood.
    this.cable = [this.halfX / 2, -(boards.pico.length / 2 + this.halfY) / 2];
    if (this.halfY - boards.pico.length / 2 < h.cableHole + 1) {
      throw new Error("head: no room for the cable's slot under the Pico: check head.cableHole");
    }
    if (
      this.cable[0] - h.cableSlotLength / 2 < h.tabWidth / 2 + 1 ||
      this.cable[0] + h.cableSlotLength / 2 > this.halfX - 1
    ) {
      throw new Error("head: the cable's slot reaches a tab or the wall: check head.cableSlotLength");
    }
  }

  /** The shell, as built: front at z = 0, its walls up to the lid. */
  get shell(): ScadObject {
    const f = this.faceplate;
    return union(
      difference(
        this.rounded(-this.outerX, this.outerX, this.outerY, 0, this.lidFace),
        box(-this.halfX, this.halfX, -this.halfY, this.halfY, this.plate, this.lidFace + 1),
        this.displayed(union(f.displayWindow, f.displayPockets)),
        this.usbNotch,
        this.hookSlots([0]),
      ),
      this.displayStakes,
    );
  }

  /** The lid, as built: from lidFace to the back, its tabs, the Pico's standoffs and their pins toward the front. */
  get lid(): ScadObject {
    const { head: h, clearance } = this.c;
    const lx = this.halfX - clearance;
    const half = (h.cableSlotLength - h.cableHole) / 2;
    const [cx, cy] = this.cable;
    return difference(
      union(
        this.rounded(-this.outerX, this.outerX, this.outerY, this.lidFace, this.depth),
        this.tabs([0]),
        this.picoMount,
      ),
      this.cableSlot([cx - half, cy], [cx + half, cy]),
      this.vents(-lx, lx, [
        ...this.picoKeepOut,
        ...this.tabsKeepOut([0]),
        [cx - h.cableSlotLength / 2, cx + h.cableSlotLength / 2, cy - h.cableHole / 2, cy + h.cableHole / 2],
      ]),
    );
  }

  /** Each part to print, as it lies on the bed. */
  get print(): Record<string, ScadObject> {
    return {
      "head-shell": this.shell,
      "head-lid": this.lid.rotate([180, 0, 0]).translate([0, 0, this.depth]),
    };
  }

  /** Everything in place: for the pictures, never printed. */
  get assembly(): ScadObject {
    return union(this.shell.color("gold"), this.stackMockup, this.lid.color("silver"));
  }
}
