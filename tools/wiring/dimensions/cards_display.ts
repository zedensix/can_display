// The Dimensions tab's display card: the board, its panel and lit area,
// the solder joints the front is relieved under.

import type { Config } from "../../../faceplate/shared/config";
import { type CardSpec, HOLE, PCB, partDrawing } from "./drawing";

export function displayCard(d: Config["display"]): CardSpec {
  const W = d.pcbWidth,
    L = d.pcbLength;
  const holeX = d.holesApartX / 2;
  const holeTop = -(L - d.holesApartY) / 2;
  const glassTop = -d.glassFromPcbTop;
  const glassBottom = glassTop - d.glassLength;
  const litTop = glassTop - d.activeFromGlassTop;
  const front = partDrawing(
    "Front, soldered header at the bottom, ribbon at the top",
    [-W / 2 - 8, -L - 8, W / 2 + 8, 6],
    (g) => {
      g.rect(-W / 2, -L, W, L, { ...PCB, key: "display.pcbWidth display.pcbLength" });
      g.rect(-d.glassWidth / 2, glassBottom, d.glassWidth, d.glassLength, {
        fill: "#020617",
        key: "display.glassWidth display.glassLength display.glassFromPcbTop",
      });
      g.rect(-d.glassWidth / 2, glassTop, d.glassWidth, d.ribbonAllowance, {
        fill: "#f59e0b",
        opacity: 0.6,
        key: "display.ribbonAllowance",
        hover: true,
      });
      g.rect(-d.activeWidth / 2, litTop - d.activeLength, d.activeWidth, d.activeLength, {
        fill: "#ff5a1e",
        opacity: 0.35,
        key: "display.activeWidth display.activeLength display.activeFromGlassTop",
      });
      for (const x of [-holeX, holeX])
        for (const y of [holeTop, holeTop - d.holesApartY])
          g.circle(x, y, d.holeDiameter / 2, {
            ...HOLE,
            key: "display.holesApartX display.holesApartY display.holeDiameter",
          });
      for (const y of [-L + 0.4, -1.6 - 0.4])
        g.rect(-d.solderRowWidth / 2, y, d.solderRowWidth, 1.6, {
          fill: "#d4a017",
          key: "display.solderRowWidth display.solderHeight",
        });
      g.text(0, litTop - d.activeLength / 2, "lit area", { fill: "#fff" });
      g.across(-W / 2, W / 2, 3, W, { key: "display.pcbWidth" });
      g.along(-L, 0, -W / 2 - 4, L, { key: "display.pcbLength" });
      g.along(glassBottom, glassTop, W / 2 + 5, d.glassLength, { key: "display.glassLength" });
      g.across(-d.solderRowWidth / 2, d.solderRowWidth / 2, -L - 4, d.solderRowWidth, {
        key: "display.solderRowWidth",
      });
      g.across(-holeX, holeX, holeTop - 3, d.holesApartX, { key: "display.holesApartX", hover: true });
      g.along(holeTop - d.holesApartY, holeTop, -holeX + 3, d.holesApartY, { key: "display.holesApartY", hover: true });
      g.across(-d.glassWidth / 2, d.glassWidth / 2, glassTop - 2.5, d.glassWidth, {
        key: "display.glassWidth",
        hover: true,
      });
      g.along(glassTop, 0, W / 2 + 2, d.glassFromPcbTop, { key: "display.glassFromPcbTop", hover: true });
      g.along(glassTop, glassTop + d.ribbonAllowance, -W / 2 - 1.5, d.ribbonAllowance, {
        key: "display.ribbonAllowance",
        hover: true,
      });
      g.across(-d.activeWidth / 2, d.activeWidth / 2, litTop - 4, d.activeWidth, {
        key: "display.activeWidth",
        hover: true,
      });
      g.along(litTop - d.activeLength, litTop, -d.activeWidth / 2 + 2.5, d.activeLength, {
        key: "display.activeLength",
        hover: true,
      });
      g.along(litTop, glassTop, d.activeWidth / 2 - 1.5, d.activeFromGlassTop, {
        key: "display.activeFromGlassTop",
        hover: true,
      });
    },
  );
  // Side: its top edge on the left, the screen up, the header's pins down at the back.
  const t = d.pcbThickness;
  const side = partDrawing("Side, screen up", [-9, -10, L + 7, t + d.glassThickness + 5], (g) => {
    g.rect(0, -2.5, 2.54, 2.5, { fill: "#0a0a0a", opacity: 0.5 });
    g.line(L - 2.5, -2.5, L - 2.5, -8.5, { stroke: "#a1a1aa", "stroke-width": 0.6 });
    g.rect(L - 3.8, -2.5, 2.54, 2.5, { fill: "#0a0a0a" });
    g.rect(0, 0, L, t, { ...PCB, key: "display.pcbThickness display.pcbLength" });
    g.rect(d.glassFromPcbTop, t, d.glassLength, d.glassThickness, {
      fill: "#020617",
      key: "display.glassThickness display.glassLength display.glassFromPcbTop",
    });
    for (const x of [1.2, L - 3.6])
      g.rect(x, t, 2.4, d.solderHeight, { fill: "#d4a017", key: "display.solderHeight display.solderRowWidth" });
    g.text(d.glassFromPcbTop + d.glassLength / 2, t + d.glassThickness + 1.5, "screen", { "font-size": 2.2 });
    g.text(L - 2.5, -9.8, "pins", { "font-size": 2.2 });
    g.along(0, t, -2, t, { key: "display.pcbThickness", hover: true });
    g.along(t, t + d.glassThickness, d.glassFromPcbTop - 2, d.glassThickness, {
      key: "display.glassThickness",
      hover: true,
    });
    g.along(t, t + d.solderHeight, L + 2.5, d.solderHeight, { key: "display.solderHeight", hover: true });
    g.across(0, d.glassFromPcbTop, -3.5, d.glassFromPcbTop, { key: "display.glassFromPcbTop", hover: true });
  });
  return {
    title: 'ST7789 2" display',
    sub: "Gold: the solder joints at both ends of the board's face",
    drawings: [front, side],
    rows: [
      ["Board width", W, "display.pcbWidth"],
      ["Board length", L, "display.pcbLength"],
      ["Board thickness", d.pcbThickness, "display.pcbThickness"],
      ["Holes apart, across (centers)", d.holesApartX, "display.holesApartX"],
      ["Holes apart, top to bottom (centers)", d.holesApartY, "display.holesApartY"],
      ["Holes' diameter", d.holeDiameter, "display.holeDiameter"],
      ["Panel width", d.glassWidth, "display.glassWidth"],
      ["Panel length", d.glassLength, "display.glassLength"],
      ["Panel thickness (glass and backlight)", d.glassThickness, "display.glassThickness"],
      ["Panel's top edge from the board's", d.glassFromPcbTop, "display.glassFromPcbTop"],
      ["Room for the ribbon above the panel", d.ribbonAllowance, "display.ribbonAllowance"],
      ["Lit area width", d.activeWidth, "display.activeWidth"],
      ["Lit area length", d.activeLength, "display.activeLength"],
      ["Lit area's top edge from the panel's", d.activeFromGlassTop, "display.activeFromGlassTop"],
      ["Solder joints, across", d.solderRowWidth, "display.solderRowWidth"],
      ["Solder joints, height off the board", d.solderHeight, "display.solderHeight"],
    ],
  };
}
