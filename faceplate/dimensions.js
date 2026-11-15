// Written by tools/render_faceplate.ts (mise run faceplate): don't edit.
window.FACEPLATE = {
  "config": {
    "display": {
      "pcbWidth": 37.5,
      "pcbLength": 58,
      "pcbThickness": 1.2,
      "holeInset": 3,
      "glassWidth": 35.2,
      "glassLength": 48.5,
      "glassThickness": 3,
      "glassFromPcbTop": 5,
      "ribbonAllowance": 0.5,
      "activeWidth": 31,
      "activeLength": 41.5,
      "activeFromGlassTop": 2,
      "solderRowWidth": 20,
      "solderHeight": 1.5
    },
    "screw": {
      "pilotDiameter": 1.7,
      "m3PilotDiameter": 2.5
    },
    "encoder": {
      "bushingDiameter": 7,
      "bushingLength": 6,
      "nutThickness": 0,
      "bodySize": 12.5,
      "bodyHeight": 5,
      "panelThickness": 1,
      "belowDisplay": 16,
      "holeSpacing": 13.75,
      "holesFromShaft": 7.9,
      "standoffDiameter": 5
    },
    "plate": {
      "frontSkin": 1.2,
      "margin": 8,
      "cornerRadius": 3
    },
    "window": {
      "margin": 0.5,
      "chamfer": 0.8
    },
    "tray": {
      "thickness": 2.5,
      "gap": 20,
      "postDiameter": 6.5,
      "screwPilot": 2.5,
      "screwClearance": 3.4,
      "standoffHeight": 5,
      "standoffWall": 1.8,
      "wireSlot": {
        "width": 40,
        "length": 7,
        "center": [
          0,
          -51.25
        ]
      }
    },
    "boards": {
      "pico": {
        "width": 51,
        "length": 21,
        "holesApartX": 47,
        "holesApartY": 11.2,
        "holeDiameter": 2.2,
        "center": [
          0,
          -12.5
        ],
        "usbSocketWidth": 8,
        "usbSocketDepth": 6,
        "componentsHeight": 4.5
      },
      "mcp2515": {
        "width": 40,
        "length": 28,
        "holesApartX": 35,
        "holesApartY": 23,
        "holeDiameter": 3.5,
        "center": [
          0,
          -70
        ]
      }
    },
    "converter": {
      "length": 46,
      "width": 27.5,
      "height": 14,
      "earLength": 14,
      "earWidthAtBox": 13,
      "earWidthAtEnd": 10,
      "earSlotWidth": 5,
      "earSlotDepth": 7
    },
    "clearance": 0.3
  },
  "checked": {
    "display.pcbWidth": 37.5,
    "display.pcbLength": 58,
    "display.solderRowWidth": 20,
    "display.solderHeight": 1.5,
    "encoder.bushingDiameter": 7,
    "encoder.bodySize": 12.5,
    "boards.pico.width": 51,
    "boards.pico.length": 21,
    "boards.pico.holesApartX": 47,
    "boards.pico.holesApartY": 11.2,
    "boards.pico.usbSocketWidth": 8,
    "boards.mcp2515.width": 40,
    "boards.mcp2515.length": 28,
    "boards.mcp2515.holesApartX": 35,
    "boards.mcp2515.holesApartY": 23,
    "converter.length": 46,
    "converter.width": 27.5,
    "converter.height": 14,
    "converter.earLength": 14,
    "converter.earWidthAtBox": 13,
    "converter.earWidthAtEnd": 10,
    "converter.earSlotWidth": 5,
    "converter.earSlotDepth": 7,
    "display.pcbThickness": 1.2,
    "display.holeInset": 3,
    "display.glassWidth": 35.2,
    "display.glassLength": 48.5,
    "display.glassThickness": 3,
    "display.glassFromPcbTop": 5,
    "display.ribbonAllowance": 0.5,
    "display.activeWidth": 31,
    "display.activeLength": 41.5,
    "display.activeFromGlassTop": 2,
    "encoder.bodyHeight": 5,
    "encoder.holeSpacing": 13.75,
    "encoder.holesFromShaft": 7.9,
    "encoder.standoffDiameter": 5,
    "encoder.belowDisplay": 16,
    "tray.standoffHeight": 5,
    "boards.pico.usbSocketDepth": 6,
    "boards.pico.componentsHeight": 4.5,
    "clearance": 0.3,
    "tray.gap": 20,
    "boards.pico.holeDiameter": 2.2,
    "boards.mcp2515.holeDiameter": 3.5,
    "encoder.nutThickness": 0,
    "encoder.bushingLength": 6,
    "encoder.panelThickness": 1
  },
  "printed": {
    "plateThickness": 5.4,
    "outlineWidth": 54.1,
    "outlineHeight": 96.85,
    "depth": 27.9,
    "pilots": {
      "pico": 1.7,
      "mcp2515": 2.5
    }
  }
};
