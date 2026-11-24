<!-- The Dimensions tab: each module as the printed parts are built for it
     (faceplate/shared/config.ts, through /dimensions). A value is "checked"
     when measured on the module or confirmed by a test print, else a
     "guess" to measure before printing. Each edit saves itself 0.8s after
     the last keystroke; the server rebuilds the parts and says whether the
     model takes the values. -->
<script lang="ts">
  import type { Config } from "../../faceplate/shared/config";
  import DimensionCard from "./DimensionCard.svelte";
  import { boardCard, type Printed, printedCard } from "./dimensions/cards_boards";
  import { displayCard } from "./dimensions/cards_display";
  import { encoderCard } from "./dimensions/cards_encoder";

  type Dimensions = { config: Config; checked: Record<string, number>; printed: Printed };

  // What's been typed or confirmed and not saved yet: key to value. It's
  // also kept in this browser until the server has it, so a reload or a
  // stopped server loses nothing; the page sends it again when it opens.
  const DRAFTS_KEY = "dimensionDrafts";
  function loadDrafts(): Record<string, number> {
    try {
      return JSON.parse(localStorage.getItem(DRAFTS_KEY) ?? "{}") ?? {};
    } catch {
      return {};
    }
  }

  let dimensions: Dimensions | null = $state(null);
  let missing = $state(false);
  let checked: Record<string, number> = $state({});
  const edits: Record<string, number> = $state(loadDrafts());
  let status = $state({ text: "Every value saves itself as you type.", error: false, reload: false });
  let legendHeight = $state(0);
  let saveTimer: ReturnType<typeof setTimeout> | undefined;
  let saving = false;

  const cards = $derived.by(() => {
    if (!dimensions) return [];
    const { config: c, printed } = dimensions;
    return [
      displayCard(c.display),
      encoderCard(c.encoder, printed.frontThickness, printed.encoderBoard),
      boardCard(
        "pico",
        "Raspberry Pi Pico W",
        "Behind the display, on 4 standoffs on the lid, either way up",
        c.boards.pico,
        c.boards.pico.holeDiameter - 2 * c.clearance,
        "#1f7a3c",
      ),
      boardCard(
        "mcp2515",
        "MCP2515 + TJA1050",
        "At one end of the bar, on 4 standoffs on its front",
        c.boards.mcp2515,
        c.boards.mcp2515.holeDiameter - 2 * c.clearance,
        "#1e40af",
      ),
      printedCard(c, printed),
    ];
  });

  const refused = (error: string) => ({ text: `Saved, but the model refuses these values: ${error}`, error: true, reload: false });

  $effect(() => {
    load();
  });

  async function load() {
    try {
      const response = await fetch("/dimensions");
      if (!response.ok) throw new Error(String(response.status));
      dimensions = await response.json();
      checked = dimensions?.checked ?? {};
    } catch {
      missing = true;
      return;
    }
    // The saved values, newer than the dimensions when the last rebuild was refused.
    try {
      const saved = await (await fetch("/measurements")).json();
      checked = saved.values;
      if (saved.error) status = refused(saved.error);
    } catch {}
    if (Object.keys(edits).length) saveDimensions();
  }

  function keepDrafts() {
    try {
      localStorage.setItem(DRAFTS_KEY, JSON.stringify(edits));
    } catch {}
  }

  function editDimension(key: string, value: number) {
    edits[key] = value;
    keepDrafts();
    status = { text: "Not saved yet…", error: false, reload: false };
    clearTimeout(saveTimer);
    saveTimer = setTimeout(saveDimensions, 800);
  }

  // Sends every checked value (the saved ones and the edits); the drawings
  // show the new parts after a reload.
  async function saveDimensions() {
    if (saving) {
      clearTimeout(saveTimer);
      saveTimer = setTimeout(saveDimensions, 800);
      return;
    }
    saving = true;
    const sent = { ...edits };
    status = { text: "Saving, then rebuilding the parts (about 10 s)…", error: false, reload: false };
    try {
      const response = await fetch("/measurements", {
        method: "POST",
        headers: { "content-type": "application/json" },
        body: JSON.stringify({ ...checked, ...sent }),
      });
      if (!response.ok) throw new Error(await response.text());
      const { error } = await response.json();
      checked = { ...checked, ...sent };
      for (const [key, value] of Object.entries(sent)) if (edits[key] === value) delete edits[key];
      keepDrafts();
      if (Object.keys(edits).length) status = { text: "Not saved yet…", error: false, reload: false };
      else if (error) status = refused(error);
      else status = { text: "Saved, parts rebuilt.", error: false, reload: true };
    } catch (error) {
      const why = error instanceof Error ? error.message : String(error);
      status = {
        text: `Not saved: ${why}. Kept in this browser, sent again with your next change or reload.`,
        error: true,
        reload: false,
      };
    } finally {
      saving = false;
    }
  }
</script>

<main style="--sticky-top: calc(var(--nav-height, 65px) + {legendHeight}px)">
  {#if missing}
    <p>No dimensions yet: run <code>mise run faceplate</code>, then reload.</p>
  {:else}
    <div class="legend" bind:offsetHeight={legendHeight}>
      <span><span class="chip checked">checked</span> measured on the module, or confirmed by a test print</span>
      <span><span class="chip guess">guess</span> a seller's figure or a guess: measure it, or click it if it's right</span>
      <span class="save">
        <span id="save-status" role="status" class:error={status.error}>
          {status.text}
          {#if status.reload}
            <a href="/#dimensions" onclick={() => location.reload()}>Reload</a> to redraw.
          {/if}
        </span>
      </span>
    </div>
    <div class="cards">
      {#each cards as spec (spec.title)}
        <DimensionCard {spec} {checked} {edits} onEdit={editDimension} />
      {/each}
    </div>
  {/if}
</main>
