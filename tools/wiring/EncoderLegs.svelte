<!-- The encoder board as soldered, in the diagram's style. -->
<script lang="ts">
  import { type Style, THEMES } from "./diagram/data";
  import { drawEncoderLegs } from "./diagram/encoder_legs";

  let { style }: { style: Style } = $props();
  let svg: SVGSVGElement | undefined = $state();

  $effect(() => {
    if (svg) drawEncoderLegs(svg, THEMES[style]);
  });
</script>

<svg bind:this={svg} id="encoder-legs" viewBox="0 0 1000 540" role="img" aria-label="The encoder's legs, soldered"></svg>
