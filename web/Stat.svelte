<!-- A total or a peak, big. -->
<script lang="ts">
  let { label, value, hot = false }: { label: string; value: string; hot?: boolean } = $props();
</script>

<div class="rounded-xl border border-frame bg-panel p-4">
  <div class="text-xs tracking-widest text-dim uppercase">{label}</div>
  <div class="font-display text-4xl font-bold" class:text-accent={hot}>{value}</div>
</div>
