<!-- Downloads the dash's data log over Web Bluetooth, sends it to the
     server's store, and shows each session's peaks. -->
<script lang="ts">
  import { uploadLog } from "./api";
  import { downloadLogFromDash, type Progress } from "./dash";
  import { groupSessions, lowestAndHighest, minutesDriven, parseDataLog, type Sample, summarize, toCsv } from "./datalog";
  import Stat from "./Stat.svelte";
  import SummaryCard from "./SummaryCard.svelte";

  let samples: Sample[] = $state([]);
  let downloaded = $state(false);
  let busy = $state(false);
  let status = $state("");
  let progress: Progress | null = $state(null);
  let stored = $state("");

  const sessions = $derived(groupSessions(samples));
  const oil = $derived(lowestAndHighest(samples, "oil"));
  const rpm = $derived(lowestAndHighest(samples, "rpm"));
  const minutes = $derived(sessions.reduce((sum, s) => sum + minutesDriven(s.samples), 0));

  async function download() {
    if (!navigator.bluetooth) {
      status = "No Web Bluetooth here: open this page in Chrome on Android.";
      return;
    }
    busy = true;
    try {
      const bytes = await downloadLogFromDash(
        (text) => (status = text),
        (p) => (progress = p),
      );
      samples = parseDataLog(bytes);
      downloaded = true;
      status = `${samples.length ? `${samples.length} samples.` : "The log is empty."} The dash is back on its gauges.`;
      if (samples.length) {
        stored = "Saving on the server…";
        try {
          const { added } = await uploadLog(bytes);
          stored = added ? `Saved on the server: ${added} new samples.` : "The server already had all of it.";
        } catch (e) {
          stored = `Not saved on the server (${e instanceof Error ? e.message : String(e)}): save the CSV.`;
        }
      }
    } catch (e) {
      status = `Failed: ${e instanceof Error ? e.message : String(e)}`;
    }
    busy = false;
  }

  function saveCsv() {
    const link = document.createElement("a");
    link.href = URL.createObjectURL(new Blob([toCsv(samples)], { type: "text/csv" }));
    link.download = "gt86-log.csv";
    link.click();
    URL.revokeObjectURL(link.href);
  }
</script>

<div>
  <section class="mt-6 rounded-2xl border border-frame bg-panel/90 p-5 shadow-[0_0_40px_rgb(255_90_30/0.08)]">
    <ol class="grid gap-3 text-sm text-dim sm:grid-cols-3">
      {#each ["On the dash: Settings, Send log.", "Bluetooth on, then tap the button and pick GT86 DASH.", "Keep this page open until the download ends."] as step, i (step)}
        <li class="flex gap-3"><span class="font-display text-xl font-bold text-accent">{i + 1}</span>{step}</li>
      {/each}
    </ol>

    <button
      type="button"
      disabled={busy}
      onclick={download}
      class="mt-5 w-full rounded-xl bg-linear-to-r from-accent to-hot py-4 font-display text-xl font-bold tracking-widest uppercase shadow-[0_0_30px_rgb(255_42_16/0.45)] transition active:scale-[0.98] disabled:opacity-40 disabled:shadow-none"
    >
      Connect and download
    </button>

    {#if progress}
      <div class="mt-5">
        <div class="relative h-3 overflow-hidden rounded-full bg-frame">
          <div
            class="relative h-full overflow-hidden rounded-full bg-linear-to-r from-accent to-hot transition-[width] duration-200"
            class:sheen={busy}
            style:width="{progress.total ? Math.min(100, (100 * progress.got) / progress.total) : 0}%"
          ></div>
        </div>
        <div class="mt-2 flex justify-between font-display text-lg text-dim">
          <span>{Math.round(progress.got / 1024)} / {Math.round(progress.total / 1024)} KB</span>
          <span>{progress.seconds ? `${(progress.got / 1024 / progress.seconds).toFixed(1)} KB/s` : ""}</span>
        </div>
      </div>
    {/if}

    <p class="mt-3 min-h-6 text-sm text-dim">{status}</p>
    {#if stored}
      <p class="text-sm text-dim">{stored} <a href="#/logs" class="font-semibold text-accent">See every session</a></p>
    {/if}
  </section>

  {#if downloaded}
    <section class="mt-6">
      <div class="grid grid-cols-2 gap-3 md:grid-cols-4">
        <Stat label="Sessions" value={String(sessions.length)} />
        <Stat label="Engine running" value="{minutes} min" />
        <Stat label="Hottest oil" value={oil ? `${oil[1].toFixed(0)}°C` : "--"} hot />
        <Stat label="Top RPM" value={rpm ? rpm[1].toFixed(0) : "--"} />
      </div>

      <button
        type="button"
        disabled={samples.length === 0}
        onclick={saveCsv}
        class="mt-4 w-full rounded-xl border border-accent bg-panel py-3 font-display text-lg font-semibold tracking-wider uppercase transition active:scale-[0.98] disabled:opacity-30"
      >
        Save CSV
      </button>

      <div class="mt-6 grid gap-4 md:grid-cols-2 xl:grid-cols-3">
        {#each sessions as session (session.id)}
          <SummaryCard summary={summarize(session)} />
        {/each}
      </div>
    </section>
  {/if}
</div>
