<!-- The wiring diagram, panned and zoomed through its viewBox, the drawing
     vector at any zoom: drag to move, cmd+scroll (ctrl+scroll, a trackpad
     pinch) to zoom at the pointer, scroll to move once zoomed in (at full
     size it scrolls the page), double-click to fit. From half size to
     20x, the view's center never off the drawing. A click on a board
     turns it over; on a wire, dims every other net, again shows them all. -->
<script lang="ts">
  import type { Style, WiringView } from "./diagram/data";
  import { drawDiagram } from "./diagram/draw";

  let { style, view, onTurnOver }: { style: Style; view: WiringView; onTurnOver: (id: string) => void } = $props();

  const FULL = { x: 0, y: 0, w: 1240, h: 820 };
  const MIN_ZOOM = 0.5;
  const MAX_ZOOM = 20;

  let svg: SVGSVGElement | undefined = $state();
  let box = $state({ ...FULL });
  let selectedNet: string | null = $state(null);
  let drag: {
    clientX: number;
    clientY: number;
    box: typeof FULL;
    moved: boolean;
    net: string | null;
    module: string | null;
  } | null = $state(null);

  // Redrawn when the style, the view or the selection changes: the
  // selected net's wires stay, the others fade.
  $effect(() => {
    if (!svg) return;
    drawDiagram(svg, style, view);
    for (const wire of svg.querySelectorAll<SVGElement>(".wire")) {
      wire.style.opacity = selectedNet && wire.dataset.net !== selectedNet ? "0.12" : "";
    }
  });

  const clampWidth = (w: number) => Math.min(FULL.w / MIN_ZOOM, Math.max(FULL.w / MAX_ZOOM, w));

  function setBox(x: number, y: number, width: number) {
    const w = clampWidth(width);
    const h = (w * FULL.h) / FULL.w;
    box = {
      x: Math.min(FULL.w - w / 2, Math.max(-w / 2, x)),
      y: Math.min(FULL.h - h / 2, Math.max(-h / 2, y)),
      w,
      h,
    };
  }

  // Diagram units per screen pixel at the current zoom.
  const unitsPerPixel = () => (svg ? box.w / svg.getBoundingClientRect().width : 1);

  function zoomAt(clientX: number, clientY: number, factor: number) {
    if (!svg) return;
    const rect = svg.getBoundingClientRect();
    const px = box.x + (clientX - rect.left) * unitsPerPixel();
    const py = box.y + (clientY - rect.top) * unitsPerPixel();
    const w = clampWidth(box.w / factor);
    const k = w / box.w;
    setBox(px - (px - box.x) * k, py - (py - box.y) * k, w);
  }

  // Not a Svelte handler: it must not be passive, to stop the page scrolling.
  $effect(() => {
    if (!svg) return;
    const wheel = (e: WheelEvent) => {
      if (e.metaKey || e.ctrlKey) {
        // A trackpad pinch arrives as ctrl+wheel with small deltas; a mouse
        // wheel notch is about 100, so cap each step to about 20%.
        const step = Math.max(-20, Math.min(20, e.deltaY));
        zoomAt(e.clientX, e.clientY, Math.exp(-step * 0.01));
      } else if (box.w !== FULL.w || box.x !== FULL.x || box.y !== FULL.y) {
        setBox(box.x + e.deltaX * unitsPerPixel(), box.y + e.deltaY * unitsPerPixel(), box.w);
      } else {
        return;
      }
      e.preventDefault();
    };
    svg.addEventListener("wheel", wheel, { passive: false });
    return () => svg?.removeEventListener("wheel", wheel);
  });

  // A press that moves more than a few pixels is a drag, else a click.
  function down(e: PointerEvent) {
    const target = e.target as SVGElement;
    drag = {
      clientX: e.clientX,
      clientY: e.clientY,
      box,
      moved: false,
      net: target.dataset.net ?? null,
      module: (target.closest("[data-module]") as SVGElement | null)?.dataset.module ?? null,
    };
    svg?.setPointerCapture(e.pointerId);
  }

  function move(e: PointerEvent) {
    if (!drag || !svg) return;
    const dx = e.clientX - drag.clientX;
    const dy = e.clientY - drag.clientY;
    if (!drag.moved && Math.hypot(dx, dy) < 4) return;
    drag.moved = true;
    const scale = drag.box.w / svg.getBoundingClientRect().width;
    setBox(drag.box.x - dx * scale, drag.box.y - dy * scale, drag.box.w);
  }

  function up() {
    if (drag && !drag.moved) {
      if (drag.module) onTurnOver(drag.module);
      else selectedNet = drag.net && drag.net !== selectedNet ? drag.net : null;
    }
    drag = null;
  }
</script>

<svg
  bind:this={svg}
  id="diagram"
  class:dragging={drag?.moved}
  viewBox="{box.x} {box.y} {box.w} {box.h}"
  role="img"
  aria-label="Wiring diagram"
  onpointerdown={down}
  onpointermove={move}
  onpointerup={up}
  onpointercancel={() => (drag = null)}
  ondblclick={() => setBox(FULL.x, FULL.y, FULL.w)}
></svg>
