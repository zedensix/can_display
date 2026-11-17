# The enclosures

Three ways to house the dash, each a model in TypeScript (scad-js) from
the modules' dimensions, printed with no supports, built by
`mise run faceplate` into each one's STLs and pictures:

- [The faceplate and the tray](plate/README.md): the display in portrait
  and the encoder on a plate, the boards on a tray behind it, all in one.
- [The vent pod](pod/README.md): all in one in the driver's round air vent,
  in place of a 52 mm gauge.
- [The head](head/README.md): the smallest box, for up high, the display
  in landscape and the Pico behind it, the rest on a cable.

The modules' dimensions are shared by the three; each model has its own
on top of them.

The wiring page's Dimensions tab (`bun run wiring`) draws every module
with the dimensions the parts are built for, marks which were measured,
and is where you enter them; hovering a row highlights what it measures.
Each value saves itself as you type it, to `data/measured.json` (it overrides
`config.ts`'s defaults), then the parts rebuild. A value the model
refuses stays saved: the page says which keys to look at.

## Fit

The default dimensions come from sellers' listings, which disagree with
each other. Before printing, measure your modules with calipers and
enter them in the Dimensions tab (or `data/measured.json`; every key is
described in [`shared/config.ts`](shared/config.ts)). These matter most:

1. The display board's width and length, and its mounting holes' distance
   from the edges.
2. The panel (glass and backlight): its width, length and thickness, and
   where its top edge sits on the board.
3. The lit area's top edge, measured from the panel's top edge: it places
   the window.
4. If the encoder's collar is threaded: its length and the nut's
   thickness (none: leave the nut at 0).
5. The encoder's body height (board to face, where the bushing starts)
   and its board's holes: they set the standoffs it screws onto.
6. The encoder's board (length, width) and where its pin row sits from
   the shaft; `encoder.panelThickness`, the plate under its body, sets
   how far the collar and the shaft stick out.

A board's 4 holes sit on a rectangle centered on it: measure them center
to center, along its width and along its length.

Then `mise run faceplate` and look at the pictures. The build refuses a
layout that can't work and names the key to fix: a standoff off the tray,
on a tray screw, a plate too thick for the encoder's
nut or too thin for a channel under the encoder's pins, the Pico's pin
slots cutting into a screw hole or a rib into its micro-USB plug, the converter on a board or a tray
screw. The first print is a test fit: `clearance` sets the
play in every pocket and hole.
