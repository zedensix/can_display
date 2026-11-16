<!-- The page: download the dash's log, browse the stored ones, a
     session's charts; light, dark or the device's theme. -->
<script lang="ts">
  import logo from "../86.png";
  import DownloadPage from "./DownloadPage.svelte";
  import LogsPage from "./LogsPage.svelte";
  import { routeFromHash } from "./routes";
  import SessionPage from "./SessionPage.svelte";
  import ThemeToggle from "./ThemeToggle.svelte";

  let route = $state(routeFromHash(location.hash));

  $effect(() => {
    const follow = () => (route = routeFromHash(location.hash));
    addEventListener("hashchange", follow);
    return () => removeEventListener("hashchange", follow);
  });

  const tabs = [
    { href: "#/", label: "Download", active: () => route.page === "download" },
    { href: "#/logs", label: "Logs", active: () => route.page !== "download" },
  ];
</script>

<div class="w-full px-4 pt-6 pb-10 sm:px-8">
  <header class="flex flex-wrap items-center gap-4">
    <img
      src={logo}
      alt=""
      class="size-16 shrink-0 rounded-full bg-white p-1 shadow-[0_0_30px_rgb(255_90_30/0.55)] ring-2 ring-accent"
    />
    <div class="mr-auto">
      <h1 class="font-display text-3xl font-bold tracking-widest text-accent">GT86 DASH</h1>
      <p class="text-sm text-dim">The data log, straight off the dash</p>
    </div>
    <ThemeToggle />
  </header>

  <nav class="mt-5 flex gap-2">
    {#each tabs as tab (tab.href)}
      <a
        href={tab.href}
        class="rounded-full border px-4 py-1.5 font-display text-lg font-semibold tracking-wider uppercase transition {tab.active()
          ? 'border-accent bg-accent text-white'
          : 'border-frame text-dim hover:text-fg'}"
      >
        {tab.label}
      </a>
    {/each}
  </nav>

  {#if route.page === "session"}
    <SessionPage id={route.id} />
  {:else if route.page === "logs"}
    <LogsPage />
  {:else}
    <DownloadPage />
  {/if}
</div>
