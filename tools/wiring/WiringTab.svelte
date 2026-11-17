<!-- The wiring tab: the diagram, the encoder as soldered, what to know. -->
<script lang="ts">
  import Diagram from "./Diagram.svelte";
  import type { Style, WiringView } from "./diagram/data";
  import EncoderLegs from "./EncoderLegs.svelte";

  let { style, view, onTurnOver }: { style: Style; view: WiringView; onTurnOver: (id: string) => void } = $props();

  const notes: [string, string][] = [
    [
      "Never 12V on the Pico",
      "Only through the 12V to 5V converter, its micro-USB plug in the Pico's socket. Take a switched 12V (ACC/ignition fuse tap), not OBD pin 16: it stays live with the car off.",
    ],
    [
      "Encoder on its legs",
      "Wires soldered straight on the EC11's legs, the KY-040 board's header unused: the Pico's internal pull-ups on GP2-4 replace the board's, nothing powers it. Turning the wrong way: swap A and B.",
    ],
    [
      "Keep GP15 free",
      "The pico-sdk borrows it at boot (RP2040-E5). GPIO23, 24, 25 and 29 belong to the wireless chip.",
    ],
    [
      "OBD dongle: no wire",
      "The Vgate iCar Pro 2S plugs into the OBD port; the Pico W talks to it over BLE. One client at a time: a phone connected to it hides it.",
    ],
    [
      "MCP2515: a divider on SO",
      "VCC on VBUS (5V): its TJA1050 transceiver needs it, so its SO swings to 5V. Never straight into GP12: 1kΩ from SO to GP12, 2kΩ (or 2.2kΩ) from GP12 to GND, 3.3V at the pin. SCK, SI and CS straight are out of spec: at 5V the MCP2515 wants 3.5V for a high, the Pico gives 3.3V (often works, not guaranteed). A 4-channel BSS138 level shifter on SCK, SI, CS and SO (HV 5V, LV 3V3) does all four right, the divider then unneeded. Without resistors: swap the TJA1050 (the 8-pin chip by the screw terminal) for an SN65HVD230, same pinout, and power the board from 3V3.",
    ],
    [
      "MCP2515 (wired CAN)",
      "Remove jumper J1 (120Ω termination). 8MHz crystal, 500kbps. On the OBD cable: CAN-H pin 6 green, CAN-L pin 14 green/white (the seller's chart: check with a multimeter); never pin 16, red, the battery's 12V.",
    ],
    [
      "Flashing in the car",
      "The Pico has one USB socket: unplug the converter's micro-USB, plug the Mac's cable instead.",
    ],
  ];
</script>

<main>
  <Diagram {style} {view} {onTurnOver} />
  <EncoderLegs {style} />
  <div class="notes">
    {#each notes as [title, text] (title)}
      <div class="note"><b>{title}</b>{text}</div>
    {/each}
  </div>
</main>
