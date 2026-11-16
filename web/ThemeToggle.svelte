<!-- Light, dark, or the device's theme. -->
<script lang="ts">
  import { applyTheme, loadTheme, THEME_MODES, type ThemeMode } from "./theme";

  let mode: ThemeMode = $state(loadTheme());

  $effect(() => {
    applyTheme(mode);
    // Following the device: its changes apply at once.
    if (mode !== "system") return;
    const media = matchMedia("(prefers-color-scheme: dark)");
    const follow = () => applyTheme("system");
    media.addEventListener("change", follow);
    return () => media.removeEventListener("change", follow);
  });
</script>

<fieldset class="flex rounded-full border border-frame bg-panel p-0.5 text-xs" aria-label="Theme">
  {#each THEME_MODES as option (option)}
    <button
      type="button"
      aria-pressed={mode === option}
      onclick={() => (mode = option)}
      class="rounded-full px-3 py-1 font-semibold capitalize transition {mode === option
        ? 'bg-accent text-white'
        : 'text-dim'}"
    >
      {option}
    </button>
  {/each}
</fieldset>
