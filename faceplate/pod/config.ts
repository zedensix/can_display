// Dimensions (mm) of the vent pod, part of shared/config.ts's (seen from the back
// of the dash like the rest), which data/measured.json overrides.
// The pod: the whole dash in the driver's round air vent, in place of a
// 52 mm gauge (pod.ts). Its front takes the outside of the gauge's
// printed adapter (pod/resources/gauge_vent.stl): its 73 mm lip on
// the driver's side, the display in landscape and the encoder under it,
// the 83 mm body into the vent's OEM ring, the 2 ears at the back.
// Behind, a can goes into the vent's duct, a sled in it carrying the
// Pico, the MCP2515 and the converter. Seen from the back like the rest:
// x across, y up, the front face at z = 0, z into the vent.
export const pod = {
  bodyDiameter: 83, // the adapter's, which the vent's OEM ring holds
  lipDiameter: 73, // its front, standing out of the vent
  lipLength: 2.2, // then a 45 degree step out to bodyDiameter (the adapter's is steeper: this one prints front down)
  depth: 24.5, // from the front face to the back, where the ears are
  earWidth: 3, // the 2 ears at the back edge, left and right: across
  earOut: 1.5, // how far they stand out of the body
  earThickness: 2.5, // front to back
  wall: 2, // the body's wall around the room behind the face
  displayY: 2, // the display board's middle, above the pod's center
  shaftY: -24, // the encoder's shaft, below the pod's center
  // The can: into the vent's duct (15 cm and more free behind the
  // adapter), onto a spigot on the pod's back, 2 countersunk M3 screws
  // through its front band into the spigot.
  canDiameter: 52,
  canWall: 1.6,
  canBandLength: 8, // its front end, thicker for the screws' countersinks
  canBandWall: 2.5,
  capThickness: 2.5, // its back end, the cable's hole in it
  cableHole: 9,
  spigotLength: 4,
  spigotWall: 3,
  screwHead: 6, // the countersunk M3's head
  // The sled: a plate across the can's middle, sliding in between 2
  // pairs of ribs, held between the spigot and the can's back. On top
  // the Pico (USB toward the front) then the MCP2515 (terminal toward
  // the back), under it the converter, zip-tied.
  plateThickness: 3,
  ribDepth: 1.4, // the ribs, from the can's wall inward
  ribWidth: 1.2,
  zipSlotWidth: 3, // the zip ties' slots through the plate
  zipSlotLength: 5,
  notchDepth: 5, // notches in the plate's edges for the wires between its 2 sides
  notchLength: 8,
  gap: 4, // along the sled, between the boards
  // The vents, a honeycomb like the adapter's grille (podvents.ts).
  ventCell: 1.8, // each cell, center to corner
  ventWall: 0.8, // between two cells: 2 lines of a 0.4 mm nozzle
};
