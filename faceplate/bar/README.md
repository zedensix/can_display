# The bar

![Bar, front](renders/bar-front.png) ![Bar, the lid off](renders/bar-inside.png)

The modules in one long, thin box laid on the steering column's shroud,
along its length instead of stacked: 164 x 41 x 19 mm (46.5 at the ends,
over the arc), the display in the
middle. On one side the encoder, its knob out the front, then the
converter's micro-USB plug in the Pico's socket; on the other the MCP2515.
The display is in landscape with the Pico right behind it, built as
[the head's](../head/README.md). The converter stays out, on the cable by
the fuse box. Two parts, printed with no supports:

- **The shell** (`bar-shell`) holds the display and the encoder as the
  faceplate does (the window, the pockets, the encoder's recess and its
  board's standoffs), its front as thick as they need there and thin under
  the MCP2515. No screw: the display's board goes onto 4 pins through its
  corner holes, the encoder's onto 2 through its 2 holes (on the standoffs
  on one side of its shaft, the other 2 hold it level: turn the board so
  its holes land on the pins), the MCP2515 onto 4 standoffs whose pins go
  through its holes: flatten each pin's tip into a head with the soldering
  iron. `encoder.holeDiameter` (3 mm) is a guess: measure it. Its bottom is hollowed into an arc, its axis front to back like the
  column's, to sit on the shroud: `bar.shroudSag` deeper at the ends than
  in the middle, VHB tape on it (not over a hook's slot). It prints front down.
- **The lid** (`bar-lid`) clips on with 3 pairs of tabs, their hooks 1.5
  mm into the walls, a lip 2 mm deep under it inside the walls all round
  (`bar.lipHeight`, broken around each tab) to stiffen it: to take it off,
  push the hooks of one side back in through their slots (the bottom ones
  through the arc) with a small screwdriver and lift that side. The Pico
  stands on it as in the head, either way up. Over the plug, an opening the
  converter's cable goes out through: pull the plug out through it to plug
  the computer in and flash, the lid on. CAN-H and CAN-L leave through the
  slot over the MCP2515's terminal. A honeycomb lets the heat out. It prints
  outside down.

The plug gets what's left between the encoder and the display once the
display sits in the middle (29 mm, `bar.plugRoomMin` at least). The
MCP2515 lies with its header toward the display and its terminal toward
the end. Its terminal block is taken as 10 mm tall (`bar.mcpTallest`) over
standoffs 3 mm tall: measure yours, the build refuses one that hits the
lid.

**Print `bar-arcs` first** to pick the bottom's arc: a strip as long as
the bar for each of 1, 1.5, 2, 2.5 and 3 mm deeper at the ends than in the
middle (`bar.gaugeSags`), as many notches on its top as its rank. Lay each
on the shroud, keep the one that sits on it without rocking or gapping,
and set `bar.shroudSag` to it.

The Pico sits behind the display, not beside it: the box is as thick
either way, and beside it would add 55 mm of length.

PLA is for the test print; the one for the car in PETG, or better ASA.
`mise run bar` opens it in OpenSCAD. The dimensions are in
[`config.ts`](config.ts), the display's, the Pico's and the lid's in the
head's.
