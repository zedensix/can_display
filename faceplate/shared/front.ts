// The front the display and the encoder sit in: the display behind a
// window, the encoder below it, in the frame of the faceplate they were
// first built for: the display in portrait, the front face at z = 0, the
// back at z = thickness; seen from the back, X across to the right, Y up
// (the display board's top edge at y = 0). The bar (bar/bar.ts) turns its
// cutouts to landscape and places them. It prints front face down: the
// pockets open upward, the window bevel a 45 degree overhang, no supports.

import type { ScadObject } from "scad-js";
import { cylinder, hull, union } from "scad-js";
import type { Config } from "./config";
import { box } from "./geometry";

export class Front {
  /** Just thick enough to hold the panel and its board, the board flush with the back. */
  readonly thickness: number;
  /** Where the panel's top edge, and the lit area's, sit. */
  readonly glassTop: number;
  readonly activeTop: number;
  readonly glassPocketBottom: number;
  readonly glassPocketTop: number;
  readonly shaftY: number;
  readonly encoderRecessHalf: number;
  /** Where the encoder's board sits, on its standoffs, and its face, where the collar starts. */
  readonly encoderBoardFace: number;
  readonly encoderBodyFace: number;
  /** The encoder board's standoff centers, in the front's X and Y. */
  readonly encoderBoardStandoffs: [number, number][];
  /** The encoder board's ends along X, centered on the shaft: its pins near the right one, seen from the back. */
  readonly encoderBoardLeft: number;
  readonly encoderBoardRight: number;

  constructor(readonly c: Config) {
    const { display, encoder, front, window, clearance } = c;
    this.thickness = front.skin + display.glassThickness + display.pcbThickness;
    this.glassTop = -display.glassFromPcbTop;
    this.activeTop = this.glassTop - display.activeFromGlassTop;
    // The ribbon folds around the panel's top edge.
    this.glassPocketBottom = this.glassTop - display.glassLength - clearance;
    this.glassPocketTop = this.glassTop + display.ribbonAllowance + clearance;
    this.shaftY = -display.pcbLength - encoder.belowDisplay;
    this.encoderRecessHalf = encoder.bodySize / 2 + clearance;
    this.encoderBodyFace = encoder.panelThickness;
    this.encoderBoardFace = this.encoderBodyFace + encoder.bodyHeight;
    this.encoderBoardStandoffs = [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [
        (sx * encoder.holeSpacing) / 2 + encoder.holesTowardPins,
        this.shaftY + sy * encoder.holesFromShaft,
      ]),
    );
    this.encoderBoardRight = encoder.boardLength / 2;
    this.encoderBoardLeft = -encoder.boardLength / 2;

    // A layout that can't work stops the build with the key to fix.
    if (
      display.activeWidth > display.glassWidth ||
      display.activeFromGlassTop + display.activeLength > display.glassLength
    ) {
      throw new Error("display.active* must fit within display.glass*");
    }
    if (window.chamfer > front.skin) {
      throw new Error("window.chamfer must be at most front.skin");
    }
    if (display.glassFromPcbTop + display.glassLength + display.ribbonAllowance > display.pcbLength) {
      throw new Error("display.glassFromPcbTop + glassLength + ribbonAllowance must fit within display.pcbLength");
    }
    if (encoder.panelThickness >= this.thickness) {
      throw new Error("encoder.panelThickness must be thinner than the front");
    }
    if (this.encoderBoardFace <= this.thickness) {
      throw new Error("encoder.bodyHeight is too short: the encoder's board would sit inside the front");
    }
    if (encoder.nutThickness > 0 && encoder.bushingLength - encoder.panelThickness < encoder.nutThickness) {
      throw new Error("encoder.panelThickness leaves too little collar out the front for the nut");
    }
    if (encoder.belowDisplay - this.encoderRecessHalf - clearance < 1) {
      throw new Error("encoder.belowDisplay is too small: the encoder's recess runs into the display's pocket");
    }
    if (display.solderHeight >= display.glassThickness) {
      throw new Error("display.solderHeight must be less than display.glassThickness");
    }
  }

  /** The window onto the lit area, beveled toward the front. */
  get displayWindow(): ScadObject {
    const { display, window, front } = this.c;
    const halfWidth = display.activeWidth / 2 + window.margin;
    const top = this.activeTop + window.margin;
    const bottom = this.activeTop - display.activeLength - window.margin;
    const chamfer = window.chamfer;
    return union(
      box(-halfWidth, halfWidth, bottom, top, -1, front.skin + 0.01),
      hull(
        box(-halfWidth - chamfer, halfWidth + chamfer, bottom - chamfer, top + chamfer, -1, 0),
        box(-halfWidth, halfWidth, bottom, top, chamfer, chamfer + 0.01),
      ),
    );
  }

  /**
   * The panel's pocket (with room for its ribbon), then the board's, both
   * open at the back, and a relief under the solder joints at each end of
   * the board, between its edge and the panel's pocket.
   */
  get displayPockets(): ScadObject {
    const { display, front, clearance } = this.c;
    return union(
      box(
        -display.glassWidth / 2 - clearance,
        display.glassWidth / 2 + clearance,
        this.glassPocketBottom,
        this.glassPocketTop,
        front.skin,
        this.thickness + 1,
      ),
      box(
        -display.pcbWidth / 2 - clearance,
        display.pcbWidth / 2 + clearance,
        -display.pcbLength - clearance,
        clearance,
        front.skin + display.glassThickness,
        this.thickness + 1,
      ),
      // Pads at both ends of the board: the header's above the panel, the other row below it.
      ...[
        [-display.pcbLength - clearance, this.glassPocketBottom + 0.01],
        [this.glassPocketTop - 0.01, clearance],
      ].map(([y0, y1]) =>
        box(
          -display.solderRowWidth / 2 - clearance,
          display.solderRowWidth / 2 + clearance,
          y0,
          y1,
          front.skin + display.glassThickness - display.solderHeight,
          this.thickness + 1,
        ),
      ),
    );
  }

  /** The bushing's hole through the front, and the body's recess from the back. */
  get encoderCutouts(): ScadObject {
    const { encoder, clearance } = this.c;
    const half = this.encoderRecessHalf;
    return union(
      cylinder(this.thickness + 2, encoder.bushingDiameter / 2 + clearance, { center: false }).translate([
        0,
        this.shaftY,
        -1,
      ]),
      box(-half, half, this.shaftY - half, this.shaftY + half, this.encoderBodyFace, this.encoderBoardFace + 1),
    );
  }

  /** The encoder board's 4 standoffs, from the front's back up to the board. */
  get encoderStandoffs(): ScadObject {
    const height = this.encoderBoardFace - this.thickness;
    return union(
      ...this.encoderBoardStandoffs.map(([x, y]) =>
        cylinder(height + 0.01, this.c.encoder.standoffDiameter / 2, { center: false }).translate([
          x,
          y,
          this.thickness - 0.01,
        ]),
      ),
    );
  }
}
