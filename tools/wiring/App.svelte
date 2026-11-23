<!-- The wiring page: the diagram in color or as a blueprint, and the
     Dimensions tab, the tab in the URL's hash. -->
<script lang="ts">
  import DimensionsTab from "./DimensionsTab.svelte";
  import type { Style, WiringView } from "./diagram/data";
  import { OTHER_FACE } from "./diagram/draw";
  import WiringTab from "./WiringTab.svelte";

  type Tab = Style | "dimensions";

  // Each board's face, kept per browser.
  const VIEW_KEY = "wiringView";
  function loadView(): WiringView {
    try {
      const saved = JSON.parse(localStorage.getItem(VIEW_KEY) ?? "null");
      if (saved?.faces) return { faces: saved.faces };
    } catch {}
    return { faces: { pico: "back", display: "back", encoder: "back", mcp: "back" } };
  }

  // #color (the default), #blueprint, #dimensions.
  function tabFromHash(): Tab {
    const name = location.hash.slice(1);
    return name === "blueprint" || name === "dimensions" ? name : "color";
  }

  const view: WiringView = $state(loadView());
  let tab: Tab = $state(tabFromHash());
  let navHeight = $state(0);

  $effect(() => {
    const follow = () => (tab = tabFromHash());
    addEventListener("hashchange", follow);
    return () => removeEventListener("hashchange", follow);
  });
  $effect(() => {
    try {
      localStorage.setItem(VIEW_KEY, JSON.stringify(view));
    } catch {}
  });
  $effect(() => {
    document.body.className = tab === "blueprint" ? "blueprint" : "";
  });
  // What sticks under the nav needs its height: it wraps on narrow screens.
  $effect(() => {
    document.documentElement.style.setProperty("--nav-height", `${navHeight}px`);
  });

  function turnOver(id: string) {
    view.faces[id] = OTHER_FACE[view.faces[id]];
  }

  const tabs: [Tab, string][] = [
    ["color", "Color"],
    ["blueprint", "Blueprint"],
    ["dimensions", "Dimensions"],
  ];
</script>

<nav bind:offsetHeight={navHeight}>
  <h1>GT86 DASH WIRING</h1>
  {#if tab !== "dimensions"}
    <span class="hint">
      Drag to move · cmd+scroll or pinch to zoom · double-click to fit · click a wire to follow it, a board to turn it
      over
    </span>
    <button
      type="button"
      class:active={view.faces.pico === "back"}
      aria-pressed={view.faces.pico === "back"}
      onclick={() => turnOver("pico")}
    >
      Pico from below
    </button>
  {/if}
  {#each tabs as [name, label] (name)}
    <a href="#{name}" class:active={tab === name}>{label}</a>
  {/each}
</nav>

{#if tab === "dimensions"}
  <DimensionsTab />
{:else}
  <WiringTab style={tab} {view} onTurnOver={turnOver} />
{/if}
