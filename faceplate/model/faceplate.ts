// The faceplate: the display in portrait behind a window, the encoder
// below it, and 4 posts at the corners of its back for the tray.
//
// The front face is at z = 0, the back at z = thickness; seen from the
// back, X runs across to the right, Y up (the display board's top edge at
// y = 0). It prints the same way,
// front face down on the bed: the pockets open upward, the posts stand
// up, the window bevel is a 45 degree overhang, no supports.

import type { ScadObject } from "scad-js";
import { cube, cylinder, difference, hull, square, union } from "scad-js";
import type { Config } from "./config";

/** The encoder board's header: 5 pins (CLK, DT, SW, +, GND), in a row across the board. */
export const ENCODER_PINS = 5;
/** Every pin header's pitch. */
export const PITCH = 2.54;

/** A box from x0..x1, y0..y1, z0..z1. */
export function box(x0: number, x1: number, y0: number, y1: number, z0: number, z1: number): ScadObject {
  return cube([x1 - x0, y1 - y0, z1 - z0], { center: false }).translate([x0, y0, z0]);
}

export class Faceplate {
  /** Just thick enough to hold the panel and its board, the board flush with the back. */
  readonly thickness: number;
  /** The outline, shared with the tray. */
  readonly halfWidth: number;
  readonly top: number;
  readonly bottom: number;
  /** Centers of the 4 posts the tray screws onto. */
  readonly posts: [number, number][];
  /** Where the panel's top edge, and the lit area's, sit. */
  readonly glassTop: number;
  readonly activeTop: number;
  private readonly glassPocketBottom: number;
  private readonly glassPocketTop: number;
  readonly shaftY: number;
  private readonly encoderRecessHalf: number;
  /** Where the encoder's board sits, on its standoffs, and its face, where the collar starts. */
  readonly encoderBoardFace: number;
  readonly encoderBodyFace: number;
  /** The encoder board's standoff centers, in the plate's X and Y. */
  private readonly encoderBoardStandoffs: [number, number][];
  /** The encoder board's ends along X, centered on the shaft: its pins near the right one, seen from the back. */
  readonly encoderBoardLeft: number;
  readonly encoderBoardRight: number;

  constructor(readonly c: Config) {
    const { display, encoder, plate, window, tray, clearance } = c;
    this.thickness = plate.frontSkin + display.glassThickness + display.pcbThickness;
    this.glassTop = -display.glassFromPcbTop;
    this.activeTop = this.glassTop - display.activeFromGlassTop;
    // The ribbon folds around the panel's top edge, away from the soldered header at the bottom.
    this.glassPocketBottom = this.glassTop - display.glassLength - clearance;
    this.glassPocketTop = this.glassTop + display.ribbonAllowance + clearance;
    this.shaftY = -display.pcbLength - encoder.belowDisplay;
    this.encoderRecessHalf = encoder.bodySize / 2 + clearance;
    this.encoderBodyFace = encoder.panelThickness;
    this.encoderBoardFace = this.encoderBodyFace + encoder.bodyHeight;
    this.encoderBoardStandoffs = [-1, 1].flatMap((sx) =>
      [-1, 1].map((sy): [number, number] => [
        (sx * encoder.holeSpacing) / 2,
        this.shaftY + sy * encoder.holesFromShaft,
      ]),
    );
    this.encoderBoardRight = encoder.boardLength / 2;
    this.encoderBoardLeft = -encoder.boardLength / 2;
    const postInset = tray.postDiameter / 2 + 0.5;
    // Wide enough for the display, and for the encoder's board beside the bottom posts.
    this.halfWidth = Math.max(
      display.pcbWidth / 2 + clearance + plate.margin,
      Math.max(-this.encoderBoardLeft, this.encoderBoardRight) + 0.5 + tray.postDiameter / 2 + postInset,
    );
    this.top = clearance + plate.margin;
    this.bottom = this.shaftY - this.encoderRecessHalf - plate.margin;
    const postX = this.halfWidth - postInset;
    this.posts = [this.top - postInset, this.bottom + postInset].flatMap((y): [number, number][] => [
      [-postX, y],
      [postX, y],
    ]);

    if (
      display.activeWidth > display.glassWidth ||
      display.activeFromGlassTop + display.activeLength > display.glassLength
    ) {
      throw new Error("display.active* must fit within display.glass*");
    }
    if (window.chamfer > plate.frontSkin) {
      throw new Error("window.chamfer must be at most plate.frontSkin");
    }
    if (display.glassFromPcbTop + display.glassLength + display.ribbonAllowance > display.pcbLength) {
      throw new Error("display.glassFromPcbTop + glassLength + ribbonAllowance must fit within display.pcbLength");
    }
    const holeRadius = c.screw.pilotDiameter / 2;
    // Under 1 mm of plate between a screw and the panel's pocket, the screw splits it.
    if (
      -display.holeInset - holeRadius - 1 < this.glassPocketTop ||
      -(display.pcbLength - display.holeInset) + holeRadius + 1 > this.glassPocketBottom
    ) {
      throw new Error(
        "the display board's screw holes leave under 1 mm of plate beside the panel's pocket: check display.holeInset, glassFromPcbTop, glassLength and ribbonAllowance",
      );
    }
    if (encoder.panelThickness >= this.thickness) {
      throw new Error("encoder.panelThickness must be thinner than the plate");
    }
    if (this.encoderBoardFace <= this.thickness) {
      throw new Error("encoder.bodyHeight is too short: the encoder's board would sit inside the plate");
    }
    if (encoder.nutThickness > 0 && encoder.bushingLength - encoder.panelThickness < encoder.nutThickness) {
      throw new Error("encoder.panelThickness leaves too little collar out the front for the nut");
    }
    const pinsFloor = this.encoderBoardFace - encoder.pinsDepth;
    if (pinsFloor < plate.frontSkin) {
      throw new Error(
        `encoder.pinsDepth: the channel under the encoder's pins would come through the front, raise encoder.panelThickness to at least ${+(encoder.panelThickness + plate.frontSkin - pinsFloor).toFixed(2)} or solder the header on the board's back`,
      );
    }
    if (encoder.pinsFromShaft - PITCH / 2 - clearance < this.encoderRecessHalf + 0.5) {
      throw new Error("encoder.pinsFromShaft puts the pins' channel in the encoder's recess");
    }
    if (encoder.pinsFromShaft > this.encoderBoardRight) {
      throw new Error("encoder.pinsFromShaft puts the pin row off the board (encoder.boardLength)");
    }
    const channelHalf = (ENCODER_PINS * PITCH) / 2 + clearance;
    if (
      pinsFloor < this.thickness &&
      this.posts.some(
        ([x, y]) =>
          x + tray.postDiameter / 2 > encoder.pinsFromShaft &&
          Math.abs(y - this.shaftY) < channelHalf + tray.postDiameter / 2,
      )
    ) {
      throw new Error("the encoder pins' channel runs into a tray post: check encoder.belowDisplay or plate.margin");
    }
    if (encoder.belowDisplay - this.encoderRecessHalf - clearance < 1) {
      throw new Error("encoder.belowDisplay is too small: the encoder's recess runs into the display's pocket");
    }
    if (
      Math.max(encoder.holeSpacing / 2, encoder.holesFromShaft) - this.encoderRecessHalf <
      c.screw.pilotDiameter / 2 + 0.3
    ) {
      throw new Error(
        "encoder.holeSpacing and holesFromShaft put a screw hole in the encoder's recess (check encoder.bodySize)",
      );
    }
    if (display.solderHeight >= display.glassThickness) {
      throw new Error("display.solderHeight must be less than display.glassThickness");
    }
    if (display.solderRowWidth + 2 * clearance > 2 * (display.pcbWidth / 2 - display.holeInset - holeRadius)) {
      throw new Error("display.solderRowWidth runs into the display board's screw holes");
    }
    if (postX - tray.postDiameter / 2 < display.pcbWidth / 2 + clearance + 0.5) {
      throw new Error("plate.margin is too narrow for tray.postDiameter beside the display board");
    }
    if (postX - tray.postDiameter / 2 < this.encoderRecessHalf + 0.5) {
      throw new Error("tray.postDiameter runs the bottom posts into the encoder's recess");
    }
  }

  /** The outline, rounded at the corners, extruded through `thickness`. */
  outline(thickness: number): ScadObject {
    const r = this.c.plate.cornerRadius;
    return square([2 * this.halfWidth - 2 * r, this.top - this.bottom - 2 * r])
      .radius_offset(r)
      .linear_extrude(thickness)
      .translate([0, (this.top + this.bottom) / 2, 0]);
  }

  /** The window onto the lit area, beveled toward the front. */
  private get displayWindow(): ScadObject {
    const { display, window, plate } = this.c;
    const halfWidth = display.activeWidth / 2 + window.margin;
    const top = this.activeTop + window.margin;
    const bottom = this.activeTop - display.activeLength - window.margin;
    const chamfer = window.chamfer;
    return union(
      box(-halfWidth, halfWidth, bottom, top, -1, plate.frontSkin + 0.01),
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
  private get displayPockets(): ScadObject {
    const { display, plate, clearance } = this.c;
    return union(
      box(
        -display.glassWidth / 2 - clearance,
        display.glassWidth / 2 + clearance,
        this.glassPocketBottom,
        this.glassPocketTop,
        plate.frontSkin,
        this.thickness + 1,
      ),
      box(
        -display.pcbWidth / 2 - clearance,
        display.pcbWidth / 2 + clearance,
        -display.pcbLength - clearance,
        clearance,
        plate.frontSkin + display.glassThickness,
        this.thickness + 1,
      ),
      // Pads at both ends of the board: the header's below the panel, the other row above it.
      ...[
        [-display.pcbLength - clearance, this.glassPocketBottom + 0.01],
        [this.glassPocketTop - 0.01, clearance],
      ].map(([y0, y1]) =>
        box(
          -display.solderRowWidth / 2 - clearance,
          display.solderRowWidth / 2 + clearance,
          y0,
          y1,
          plate.frontSkin + display.glassThickness - display.solderHeight,
          this.thickness + 1,
        ),
      ),
    );
  }

  /** Pilot holes for the display board's 4 screws, blind so the front stays clean. */
  private get displayScrewHoles(): ScadObject {
    const { display, screw, plate } = this.c;
    const x = display.pcbWidth / 2 - display.holeInset;
    return union(
      ...[-x, x].flatMap((holeX) =>
        [-display.holeInset, -(display.pcbLength - display.holeInset)].map((holeY) =>
          cylinder(this.thickness, screw.pilotDiameter / 2, { center: false }).translate([
            holeX,
            holeY,
            plate.frontSkin,
          ]),
        ),
      ),
    );
  }

  /** The bushing's hole through the plate, and the body's recess from the back. */
  private get encoderCutouts(): ScadObject {
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

  /**
   * The channel under the encoder board's bent pins and their plugs, from
   * the pin row out through the plate's right edge, where the wires leave;
   * from the back down to a skin at the front.
   */
  private get encoderPinsChannel(): ScadObject {
    const { encoder, clearance } = this.c;
    const halfRow = (ENCODER_PINS * PITCH) / 2 + clearance;
    return box(
      encoder.pinsFromShaft - PITCH / 2 - clearance,
      this.halfWidth + 1,
      this.shaftY - halfRow,
      this.shaftY + halfRow,
      this.encoderBoardFace - encoder.pinsDepth,
      this.thickness + 1,
    );
  }

  /** The encoder board's 4 standoffs, from the plate's back up to the board. */
  private get encoderStandoffs(): ScadObject {
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

  /** Pilot holes through the encoder board's standoffs (2 take its screws), blind in the plate. */
  private get encoderScrewHoles(): ScadObject {
    const { plate, screw } = this.c;
    return union(
      ...this.encoderBoardStandoffs.map(([x, y]) =>
        cylinder(this.encoderBoardFace, screw.pilotDiameter / 2, { center: false }).translate([x, y, plate.frontSkin]),
      ),
    );
  }

  /** The 4 posts on the back, each with a pilot hole for the tray's screw. */
  private get trayPosts(): ScadObject {
    const { tray } = this.c;
    return difference(
      union(
        ...this.posts.map(([x, y]) =>
          cylinder(tray.gap + 0.01, tray.postDiameter / 2, { center: false }).translate([x, y, this.thickness - 0.01]),
        ),
      ),
      ...this.posts.map(([x, y]) =>
        cylinder(tray.gap, tray.screwPilot / 2, { center: false }).translate([x, y, this.thickness + 1]),
      ),
    );
  }

  get object(): ScadObject {
    return union(
      difference(
        union(this.outline(this.thickness), this.encoderStandoffs),
        this.displayWindow,
        this.displayPockets,
        this.displayScrewHoles,
        this.encoderCutouts,
        // Only if the pins sit on the knob side, lower than the plate's back.
        ...(this.encoderBoardFace - this.c.encoder.pinsDepth < this.thickness ? [this.encoderPinsChannel] : []),
        this.encoderScrewHoles,
      ),
      this.trayPosts,
    );
  }
}
