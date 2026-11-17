// Dimensions (mm) of the faceplate, the tray and the vent clamp, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
export const plateConfig = {
  plate: {
    frontSkin: 1.2, // material in front of the display glass
    margin: 8, // plate around the display board and the encoder body: wide enough for tray.postDiameter beside the board
    cornerRadius: 3, // rounding of the plate's corners, seen from the front
  },
  // A lip along the edge, on the faceplate's back and the tray's: on their fronts, they'd print over supports.
  rim: {
    height: 2,
    width: 2,
  },
  window: {
    margin: 0.5, // window larger than the lit area on each side, so no pixel is hidden
    chamfer: 0.8, // 45 degree bevel on the window's front edge, at most plate.frontSkin
  },
  // The tray screws onto 4 posts at the faceplate's corners, behind the
  // display, and carries the boards on standoffs and the converter on its
  // back. Between the two: the display's and the encoder's plugs, and the
  // Pico's through the tray.
  tray: {
    thickness: 2.5,
    gap: 20, // from the faceplate's back to the tray: the Dupont plugs
    postDiameter: 6.5, // the faceplate's posts
    screwPilot: 2.5, // hole an M3 self-tapping screw bites into, in each post
    screwClearance: 3.4, // the M3 screw's hole through the tray
    standoffWall: 1.8, // around each standoff's pilot hole
  },
  // A temporary mount on an air vent's horizontal slats, for testing on
  // the road (ventmount.ts): a clamp on one slat, from a bar across the
  // tray's back held by the 2 top tray screws (M3x14 instead of M3x10), and
  // 2 feet on the bottom posts (M3x30) against what's under the vent.
  // Guesses: measure the vent.
  vent: {
    standoff: 20, // from the tray's back to the vent's face: past the converter (14)
    slatThickness: 2, // the slat the clamp takes, top to bottom
    slatDepth: 12, // that slat, front to back, up to the vertical vanes behind it: the clamp's legs stop 1 mm short
    slatGap: 8, // between two slats: each leg goes in one, above and below the slat
    legThickness: 3, // each leg, top to bottom
    legWidth: 12, // each leg, across
    wheelDiameter: 14, // the thumbwheel on the clamp's screw, between the tray and the vent
    feetLength: 20, // from the tray's back to what the dash's bottom rests on, under the vent (the radio's face)
  },
};
