<!-- A stored session: its peaks, a chart per signal with a shared cursor,
     CSV, and forgetting it. -->
<script lang="ts">
  import { deleteSession, sessionSamples } from "./api";
  import Chart from "./Chart.svelte";
  import { type Column, minutesDriven, type Sample, summarize, toCsv } from "./datalog";
  import Stat from "./Stat.svelte";

  let { id }: { id: number } = $props();

  let samples: Sample[] = $state([]);
  let error = $state("");
  let loading = $state(true);
  let cursor: number | null = $state(null);

  $effect(() => {
    loading = true;
    sessionSamples(id)
      .then((list) => (samples = list))
      .catch((e) => (error = e instanceof Error ? e.message : String(e)))
      .finally(() => (loading = false));
  });

  const summary = $derived(summarize({ id, samples }));
  const t0 = $derived(samples[0]?.t ?? 0);
  const span = $derived(samples.length ? (samples[samples.length - 1].t - t0) / 60000 : 0);

  const CHARTS: { column: Column; label: string; unit: string; digits: number; color: string }[] = [
    { column: "rpm", label: "RPM", unit: "", digits: 0, color: "var(--accent)" },
    { column: "oil", label: "Oil", unit: "°C", digits: 0, color: "#ef4444" },
    { column: "coolant", label: "Coolant", unit: "°C", digits: 0, color: "#3b82f6" },
    { column: "intake", label: "Intake", unit: "°C", digits: 0, color: "#06b6d4" },
    { column: "afr", label: "AFR", unit: "", digits: 1, color: "#a855f7" },
    { column: "battery", label: "Battery", unit: "V", digits: 1, color: "#eab308" },
    { column: "ethanol", label: "Ethanol", unit: "%", digits: 0, color: "var(--good)" },
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
    <h2 class="font-display text-3xl font-bold tracking-wider text-accent">SESSION {id}</h2>
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
