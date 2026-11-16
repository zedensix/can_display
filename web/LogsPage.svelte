<!-- Every session the server keeps, the latest first. -->
<script lang="ts">
  import { listSessions } from "./api";
  import type { SessionSummary } from "./datalog";
  import SummaryCard from "./SummaryCard.svelte";

  let summaries: SessionSummary[] = $state([]);
  let error = $state("");
  let loading = $state(true);

  $effect(() => {
    listSessions()
      .then((list) => (summaries = list))
      .catch((e) => (error = e instanceof Error ? e.message : String(e)))
      .finally(() => (loading = false));
  });
</script>

<section class="mt-6">
  {#if error}
    <p class="text-sm text-hot">Can't reach the log store: {error}</p>
  {:else if loading}
    <p class="text-sm text-dim">Loading…</p>
  {:else if !summaries.length}
    <p class="text-sm text-dim">No log stored yet: download one from the dash.</p>
  {:else}
    <div class="grid gap-4 md:grid-cols-2 xl:grid-cols-3 2xl:grid-cols-4">
      {#each summaries as summary (summary.id)}
        <SummaryCard {summary} href="#/logs/{summary.id}" />
      {/each}
    </div>
  {/if}
</section>
