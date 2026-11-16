<!-- The stored sessions, the latest first: a search (a name, or a
     session's number), a range of days, pages of them. -->
<script lang="ts">
  import { listSessions } from "./api";
  import type { SessionPage } from "./datalog";
  import SummaryCard from "./SummaryCard.svelte";

  const SIZE = 12;

  let q = $state("");
  let fromDay = $state("");
  let toDay = $state("");
  let page = $state(1);
  let result: SessionPage = $state({ total: 0, items: [] });
  let error = $state("");
  let loading = $state(true);

  const pages = $derived(Math.max(1, Math.ceil(result.total / SIZE)));
  const filtered = $derived(Boolean(q || fromDay || toDay));
  const field =
    "rounded-xl border border-frame bg-panel px-3 py-2 text-sm text-fg placeholder:text-dim focus:border-accent focus:outline-none";

  // A day picked in this browser's time zone: from its first millisecond to its last.
  const dayStart = (day: string) => (day ? new Date(`${day}T00:00:00`).getTime() : undefined);
  const dayEnd = (day: string) => (day ? new Date(`${day}T23:59:59.999`).getTime() : undefined);

  // Each change of the filters goes back to the first page; typing waits for a pause.
  let timer: ReturnType<typeof setTimeout> | undefined;
  $effect(() => {
    const query = { q: q.trim(), from: dayStart(fromDay), to: dayEnd(toDay), page, size: SIZE };
    clearTimeout(timer);
    timer = setTimeout(
      () => {
        loading = true;
        listSessions(query)
          .then((found) => {
            result = found;
            error = "";
          })
          .catch((e) => (error = e instanceof Error ? e.message : String(e)))
          .finally(() => (loading = false));
      },
      query.q ? 250 : 0,
    );
  });

  function filter(set: () => void) {
    set();
    page = 1;
  }

  function clear() {
    filter(() => {
      q = "";
      fromDay = "";
      toDay = "";
    });
  }
</script>

<section class="mt-6">
  <div class="flex flex-wrap items-end gap-3">
    <label class="flex grow flex-col gap-1 text-xs tracking-widest text-dim uppercase">
      Search
      <input
        type="search"
        placeholder="A name, or a session's number"
        value={q}
        oninput={(e) => filter(() => (q = e.currentTarget.value))}
        class="{field} normal-case tracking-normal"
      />
    </label>
    <label class="flex flex-col gap-1 text-xs tracking-widest text-dim uppercase">
      From
      <input type="date" value={fromDay} oninput={(e) => filter(() => (fromDay = e.currentTarget.value))} class={field} />
    </label>
    <label class="flex flex-col gap-1 text-xs tracking-widest text-dim uppercase">
      To
      <input type="date" value={toDay} oninput={(e) => filter(() => (toDay = e.currentTarget.value))} class={field} />
    </label>
    {#if filtered}
      <button type="button" onclick={clear} class="rounded-xl px-3 py-2 text-sm font-semibold text-dim hover:text-accent">
        Clear
      </button>
    {/if}
  </div>

  {#if error}
    <p class="mt-4 text-sm text-hot">Can't reach the log store: {error}</p>
  {:else if !loading && !result.items.length}
    <p class="mt-4 text-sm text-dim">
      {filtered ? "No session matches." : "No log stored yet: download one from the dash."}
    </p>
  {:else}
    <div class="mt-4 grid gap-4 md:grid-cols-2 xl:grid-cols-3 2xl:grid-cols-4" class:opacity-60={loading}>
      {#each result.items as summary (summary.id)}
        <SummaryCard {summary} href="#/logs/{summary.id}" />
      {/each}
    </div>
    <nav class="mt-5 flex items-center justify-between text-sm text-dim" aria-label="Pages">
      <span>{result.total} session{result.total === 1 ? "" : "s"}</span>
      <div class="flex items-center gap-2">
        <button
          type="button"
          disabled={page <= 1}
          onclick={() => page--}
          class="rounded-full border border-frame px-4 py-1.5 font-semibold disabled:opacity-30 enabled:hover:border-accent enabled:hover:text-accent"
        >
          ← Newer
        </button>
        <span class="tabular-nums">Page {page} of {pages}</span>
        <button
          type="button"
          disabled={page >= pages}
          onclick={() => page++}
          class="rounded-full border border-frame px-4 py-1.5 font-semibold disabled:opacity-30 enabled:hover:border-accent enabled:hover:text-accent"
        >
          Older →
        </button>
      </div>
    </nav>
  {/if}
</section>
