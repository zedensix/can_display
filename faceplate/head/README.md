# The head

![Head, front](renders/head-front.png) ![Head, cut open](renders/head-side.png)

The smallest box, for up high (the meter hood's top, in the driver's line
of sight): the display in landscape and the Pico W right behind it, about
62 x 41 x 16 mm, a 20 degree wedge under it. Everything else goes on a
cable, out of sight: the MCP2515 and the converter at the OBD port (both
heat-shrunk, no box), the encoder wherever the hand falls. The Pico has
to stay by the display: its SPI runs at 62.5 MHz, which a long cable
won't carry; the MCP2515's (1 MHz) and the encoder's wires will. Two
parts, printed with no supports:

- **The shell** (`head-shell`) holds the display as the faceplate does
  (the window, the pockets), its walls just around the board, nothing
  standing into the room: the board goes in from the back, onto 4 pins
  through its corner holes. Flatten each pin's tip into a head with the
  soldering iron (a flat tip, low heat, a second or two): no screw. In the
  end by the Pico's USB socket, a notch for the plug when flashing; in the
  top and bottom walls, a slot for each of the lid's hooks. The wedge under
  it leans the screen back toward the eyes; its face takes 3M VHB tape. It
  prints front down.
- **The lid** (`head-lid`) lies on the walls' back edge all round and
  clips on: 2 tabs hang along the top and bottom walls, their hooks click
  into the slots. To take it off, push a hook back in through its slot with
  a small screwdriver and lift that side, then the other (the bottom slot
  goes through the wedge: take the head off the hood first). The Pico lies
  on the lid, components toward the display, its header's plastic on the
  lid, on 4 standoffs as tall whose pins go through its holes, flattened
  the same way; on that face the USB socket isn't between them. The 2 by
  the socket stand only 0.6 mm past the board, for a smaller head: they're
  9.6 mm apart for a socket 8 mm wide, a full head would touch it. Cut the
  long pins flush with the plastic and solder the wires on their short tips
  before melting the pins; no Dupont plug fits. The display's header gets
  the same: pins cut past the plastic, wires soldered. A honeycomb in the
  lid lets the heat out; the cable (6 wires to the OBD end, 4 to the
  encoder) leaves through the slot in it (7 x 16 mm, 2 cables), out the
  back, hidden behind the box. It prints outside down.

Between the Pico and the display, 5.5 mm: room for the wires soldered on
the Pico's pin tips and bent over, in 26 AWG silicone wire at most (30 AWG
wire-wrap wire leaves plenty); beside the Pico, 8.5 mm above and below it
for the wires to run along to the display's header and the slot.

`display.holeDiameter` is a guess (2.5 mm): measure the display board's
holes, the pins are made 0.6 mm thinner. The tabs bend about 2.4%: fine in
PLA, better in PETG or ASA.

PLA is for the test print: the top of the dash in the sun gets past 60°C,
where PLA sags. The one for the car in PETG, or better ASA. `head.tilt`
is a guess: set it from the hood's slope. `mise run head` opens it in
OpenSCAD. The dimensions are in [`config.ts`](config.ts).
