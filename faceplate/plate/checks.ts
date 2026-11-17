// The faceplate's layout checks: a layout that can't work (a pocket
// through the front, a screw in a recess, a post in the encoder's way)
// stops the build with the key to fix, before anything is printed.

import { ENCODER_PINS, PITCH } from "../shared/geometry";
import type { Faceplate } from "./faceplate";

export function checkFaceplate(f: Faceplate): void {
  const { display, encoder, plate, window, tray, clearance } = f.c;
  const postX = f.halfWidth - (tray.postDiameter / 2 + 0.5);
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
  const holeRadius = f.c.screw.pilotDiameter / 2;
  // Under 1 mm of plate between a screw and the panel's pocket, the screw splits it.
  if (
    -display.holeInset - holeRadius - 1 < f.glassPocketTop ||
    -(display.pcbLength - display.holeInset) + holeRadius + 1 > f.glassPocketBottom
  ) {
    throw new Error(
      "the display board's screw holes leave under 1 mm of plate beside the panel's pocket: check display.holeInset, glassFromPcbTop, glassLength and ribbonAllowance",
    );
  }
  if (encoder.panelThickness >= f.thickness) {
    throw new Error("encoder.panelThickness must be thinner than the plate");
  }
  if (f.encoderBoardFace <= f.thickness) {
    throw new Error("encoder.bodyHeight is too short: the encoder's board would sit inside the plate");
  }
  if (encoder.nutThickness > 0 && encoder.bushingLength - encoder.panelThickness < encoder.nutThickness) {
    throw new Error("encoder.panelThickness leaves too little collar out the front for the nut");
  }
  const pinsFloor = f.encoderBoardFace - encoder.pinsDepth;
  if (pinsFloor < plate.frontSkin) {
    throw new Error(
      `encoder.pinsDepth: the channel under the encoder's pins would come through the front, raise encoder.panelThickness to at least ${+(encoder.panelThickness + plate.frontSkin - pinsFloor).toFixed(2)} or solder the header on the board's back`,
    );
  }
  if (encoder.pinsFromShaft - PITCH / 2 - clearance < f.encoderRecessHalf + 0.5) {
    throw new Error("encoder.pinsFromShaft puts the pins' channel in the encoder's recess");
  }
  if (encoder.pinsFromShaft > f.encoderBoardRight) {
    throw new Error("encoder.pinsFromShaft puts the pin row off the board (encoder.boardLength)");
  }
  const channelHalf = (ENCODER_PINS * PITCH) / 2 + clearance;
  if (
    pinsFloor < f.thickness &&
    f.posts.some(
      ([x, y]) =>
        x + tray.postDiameter / 2 > encoder.pinsFromShaft &&
        Math.abs(y - f.shaftY) < channelHalf + tray.postDiameter / 2,
    )
  ) {
    throw new Error("the encoder pins' channel runs into a tray post: check encoder.belowDisplay or plate.margin");
  }
  if (encoder.belowDisplay - f.encoderRecessHalf - clearance < 1) {
    throw new Error("encoder.belowDisplay is too small: the encoder's recess runs into the display's pocket");
  }
  if (
    f.encoderBoardStandoffs.some(
      ([x, y]) =>
        Math.max(Math.abs(x), Math.abs(y - f.shaftY)) - f.encoderRecessHalf < f.c.screw.pilotDiameter / 2 + 0.3,
    )
  ) {
    throw new Error(
      "encoder.holeSpacing, holesFromShaft and holesTowardPins put a screw hole in the encoder's recess (check encoder.bodySize)",
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
  if (postX - tray.postDiameter / 2 < f.encoderRecessHalf + 0.5) {
    throw new Error("tray.postDiameter runs the bottom posts into the encoder's recess");
  }
}
