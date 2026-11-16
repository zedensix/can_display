<!-- A session's peaks: one drive, from a boot of the dash to the next. A
     link to its charts once the server has it. -->
<script lang="ts">
  import type { SessionSummary } from "./datalog";

  let { summary, href }: { summary: SessionSummary; href?: string } = $props();

  const value = (v: number | null, unit = "", digits = 0) => (v === null ? "--" : `${v.toFixed(digits)}${unit}`);
  const rows = $derived([
    { label: "Top RPM", value: value(summary.topRpm) },
    { label: "Hottest oil", value: value(summary.peakOil, "°C"), hot: (summary.peakOil ?? 0) >= 120 },
    { label: "Hottest coolant", value: value(summary.peakCoolant, "°C") },
    { label: "Hottest intake", value: value(summary.peakIntake, "°C") },
    {
      label: "AFR range",
      value:
        summary.afrMin === null || summary.afrMax === null
          ? "--"
          : `${summary.afrMin.toFixed(1)} to ${summary.afrMax.toFixed(1)}`,
    },
  ]);
</script>

<svelte:element
  this={href ? "a" : "article"}
  {href}
  class="block rounded-2xl border border-frame bg-panel p-4 transition {href ? 'hover:border-accent' : ''}"
>
  <header class="flex items-baseline justify-between gap-3">
    <h2 class="font-display text-2xl font-bold tracking-wider text-accent">SESSION {summary.id}</h2>
    <span class="text-sm text-dim">
      {summary.minutes} min{summary.receivedAt ? ` · ${new Date(summary.receivedAt).toLocaleDateString()}` : ""}
    </span>
  </header>
  <div class="mt-3 divide-y divide-frame">
    {#each rows as row (row.label)}
      <div class="flex items-baseline justify-between py-1.5">
        <span class="text-sm text-dim">{row.label}</span>
        <span class="font-display text-lg font-semibold tabular-nums" class:text-accent={row.hot}>{row.value}</span>
      </div>
    {/each}
  </div>
</svelte:element>
