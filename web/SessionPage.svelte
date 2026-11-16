<!-- A stored session: its name (renamed in place), its peaks, a chart per
     signal with a shared cursor, CSV, and forgetting it. -->
<script lang="ts">
  import { deleteSession, getSession, renameSession } from "./api";
  import Chart from "./Chart.svelte";
  import { type Column, MAX_NAME, minutesDriven, type Sample, summarize, toCsv } from "./datalog";
  import Stat from "./Stat.svelte";

  let { id }: { id: number } = $props();

  let samples: Sample[] = $state([]);
  let name: string | null = $state(null);
  let renaming = $state(false);
  let error = $state("");
  let loading = $state(true);
  let cursor: number | null = $state(null);

  $effect(() => {
    loading = true;
    getSession(id)
      .then((session) => {
        samples = session.samples;
        name = session.name;
      })
      .catch((e) => (error = e instanceof Error ? e.message : String(e)))
      .finally(() => (loading = false));
  });

  const summary = $derived(summarize({ id, samples }));
  const t0 = $derived(samples[0]?.t ?? 0);
  const span = $derived(samples.length ? (samples[samples.length - 1].t - t0) / 60000 : 0);

  // What the driver did, then how the engine ran (the AFR beside the one
  // the ECU commanded, the trims, the timing, the rail), then the temperatures.
  const CHARTS: { column: Column; label: string; unit: string; digits: number; color: string; minRange: number }[] = [
    { column: "rpm", label: "RPM", unit: "", digits: 0, color: "var(--accent)", minRange: 1000 },
    { column: "speed", label: "Speed", unit: " km/h", digits: 0, color: "#f97316", minRange: 20 },
    { column: "pedal", label: "Pedal", unit: "%", digits: 0, color: "#64748b", minRange: 20 },
    { column: "throttle", label: "Throttle", unit: "%", digits: 0, color: "#94a3b8", minRange: 20 },
    { column: "load", label: "Load", unit: "%", digits: 0, color: "#ec4899", minRange: 20 },
    { column: "afr", label: "AFR", unit: "", digits: 1, color: "#a855f7", minRange: 2 },
    { column: "afrTarget", label: "AFR commanded", unit: "", digits: 1, color: "#c084fc", minRange: 2 },
    { column: "stft", label: "Short term trim", unit: "%", digits: 1, color: "#14b8a6", minRange: 10 },
    { column: "ltft", label: "Long term trim", unit: "%", digits: 1, color: "#0d9488", minRange: 10 },
    { column: "timing", label: "Timing", unit: "°", digits: 1, color: "#facc15", minRange: 10 },
    { column: "rail", label: "DI rail", unit: " MPa", digits: 1, color: "#f43f5e", minRange: 4 },
    { column: "oil", label: "Oil", unit: "°C", digits: 0, color: "#ef4444", minRange: 10 },
    { column: "coolant", label: "Coolant", unit: "°C", digits: 0, color: "#3b82f6", minRange: 10 },
    { column: "intake", label: "Intake", unit: "°C", digits: 0, color: "#06b6d4", minRange: 10 },
    { column: "ambient", label: "Outside", unit: "°C", digits: 0, color: "#38bdf8", minRange: 10 },
    { column: "battery", label: "Battery", unit: "V", digits: 1, color: "#eab308", minRange: 1 },
    { column: "ethanol", label: "Ethanol", unit: "%", digits: 0, color: "var(--good)", minRange: 10 },
  ];
  // A chart per signal the session has readings of.
  const charts = $derived(
    CHARTS.map((chart) => ({
      ...chart,
      points: samples.map((s): [number, number | null] => [(s.t - t0) / 60000, s[chart.column]]),
    })).filter((chart) => chart.points.some(([, v]) => v !== null)),
  );

  function saveCsv() {
    const link = document.createElement("a");
    link.href = URL.createObjectURL(new Blob([toCsv(samples)], { type: "text/csv" }));
    link.download = `gt86-session-${id}.csv`;
    link.click();
    URL.revokeObjectURL(link.href);
  }

  // Enter or leaving the field saves; Escape doesn't (the blur that follows
  // finds renaming already over).
  async function saveName(input: HTMLInputElement) {
    if (!renaming) return;
    renaming = false;
    try {
      name = await renameSession(id, input.value);
    } catch (e) {
      error = e instanceof Error ? e.message : String(e);
    }
  }

  function renameKeys(e: KeyboardEvent & { currentTarget: HTMLInputElement }) {
    if (e.key === "Enter") saveName(e.currentTarget);
    if (e.key === "Escape") renaming = false;
  }

  async function forget() {
    if (!confirm(`Delete session ${id} from the server? The dash may still have it.`)) return;
    try {
      await deleteSession(id);
      location.hash = "#/logs";
    } catch (e) {
      error = e instanceof Error ? e.message : String(e);
    }
  }
</script>

<section class="mt-6">
  <a href="#/logs" class="text-sm text-dim hover:text-accent">← Every session</a>
  <div class="mt-2 flex flex-wrap items-baseline justify-between gap-3">
    {#if renaming}
      <input
        value={name ?? ""}
        maxlength={MAX_NAME}
        placeholder="Session {id}"
        aria-label="The session's name"
        onkeydown={renameKeys}
        onblur={(e) => saveName(e.currentTarget)}
        {@attach (input) => input.focus()}
        class="min-w-0 grow rounded-xl border border-accent bg-panel px-3 py-1 font-display text-3xl font-bold tracking-wider text-accent focus:outline-none"
      />
    {:else}
      <button
        type="button"
        title="Rename"
        onclick={() => (renaming = true)}
        class="group flex min-w-0 items-baseline gap-3 text-left"
      >
        <h2 class="truncate font-display text-3xl font-bold tracking-wider text-accent">{name ?? `SESSION ${id}`}</h2>
        <span class="shrink-0 text-xs tracking-widest text-dim uppercase group-hover:text-accent">
          {name ? `Session ${id} · ` : ""}Rename
        </span>
      </button>
    {/if}
    <div class="flex gap-2">
      <button
        type="button"
        disabled={!samples.length}
        onclick={saveCsv}
        class="rounded-xl border border-accent bg-panel px-4 py-2 font-display font-semibold tracking-wider uppercase disabled:opacity-30"
      >
        Save CSV
      </button>
      <button
        type="button"
        onclick={forget}
        class="rounded-xl border border-frame bg-panel px-4 py-2 font-display font-semibold tracking-wider text-dim uppercase hover:border-hot hover:text-hot"
      >
        Delete
      </button>
    </div>
  </div>

  {#if error}
    <p class="mt-4 text-sm text-hot">{error}</p>
  {:else if loading}
    <p class="mt-4 text-sm text-dim">Loading…</p>
  {:else}
    <div class="mt-4 grid grid-cols-2 gap-3 md:grid-cols-4">
      <Stat label="Engine running" value="{minutesDriven(samples)} min" />
      <Stat label="Top RPM" value={summary.topRpm === null ? "--" : summary.topRpm.toFixed(0)} />
      <Stat label="Hottest oil" value={summary.peakOil === null ? "--" : `${summary.peakOil.toFixed(0)}°C`} hot />
      <Stat
        label="AFR range"
        value={summary.afrMin === null || summary.afrMax === null
          ? "--"
          : `${summary.afrMin.toFixed(1)}–${summary.afrMax.toFixed(1)}`}
      />
    </div>
    <div class="mt-4 grid gap-3 lg:grid-cols-2 2xl:grid-cols-3">
      {#each charts as chart (chart.column)}
        <Chart {...chart} {span} bind:cursor />
      {/each}
    </div>
    <p class="mt-2 text-xs text-dim">
      {span.toFixed(1)} min, {samples.length} samples{cursor === null ? "" : ` · at ${cursor.toFixed(2)} min`}
    </p>
  {/if}
</section>
