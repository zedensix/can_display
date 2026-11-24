# The enclosure

[The bar](bar/README.md): one long, thin box on the steering column's
shroud, the display in landscape with the Pico behind it in the middle,
the encoder and the MCP2515 either side, the converter on the cable. It
clips into a base, the only part taped to the car. Modeled in TypeScript
(scad-js) from the modules' dimensions, printed with no supports, built
by `mise run faceplate` into its STLs and pictures.

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
described in [`shared/config.ts`](shared/config.ts) and
[`bar/config.ts`](bar/config.ts)). These matter most:

1. The display board's width and length, and its mounting holes' distance
   apart.
2. The panel (glass and backlight): its width, length and thickness, and
   where its top edge sits on the board.
3. The lit area's top edge, measured from the panel's top edge: it places
   the window.
4. If the encoder's collar is threaded: its length and the nut's
   thickness (none: leave the nut at 0).
5. The encoder's body height (board to face, where the bushing starts)
   and its board's holes: they set the standoffs it sits on.
6. The encoder's board (length, width); `encoder.panelThickness`, the
   front under its body, sets how far the collar and the shaft stick out.

A board's 4 holes sit on a rectangle centered on it: measure them center
to center, along its width and along its length.

Then `mise run faceplate` and look at the pictures. The build refuses a
layout that can't work and names the key to fix. The first print is a
test fit: `clearance` sets the play in every pocket and hole.
