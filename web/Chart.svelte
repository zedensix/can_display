<!-- One signal of a session over time: a line, breaks where the dash had
     no reading, and the value under the cursor (shared by every chart of
     the page: hovering one moves them all). -->
<script lang="ts">
  type Point = [minutes: number, value: number | null];

  let {
    label,
    unit = "",
    digits = 0,
    color,
    points,
    span,
    minRange = 0,
    cursor = $bindable(null),
  }: {
    label: string;
    unit?: string;
    digits?: number;
    color: string;
    points: Point[];
    span: number;
    // The smallest range the chart spans: a steady signal's noise stays small.
    minRange?: number;
    cursor?: number | null;
  } = $props();

  const W = 1000;
  const H = 200;
  // A long session draws the lowest and highest of each column of pixels, not every sample.
  const MAX_POINTS = 1600;

  const values = $derived(points.flatMap(([, v]) => (v === null ? [] : [v])));
  // Not Math.min(...values): a long session has more samples than a call takes arguments.
  const low = $derived(values.reduce((a, b) => Math.min(a, b), values[0] ?? 0));
  const high = $derived(values.reduce((a, b) => Math.max(a, b), values[0] ?? 1));
  const range = $derived(Math.max(high - low, minRange));
  const bottom = $derived((low + high) / 2 - range / 2 - range * 0.08);
  const top = $derived((low + high) / 2 + range / 2 + range * 0.08);
  const x = (minutes: number) => (span ? (minutes / span) * W : 0);
  const y = (v: number) => H - ((v - bottom) / (top - bottom || 1)) * H;

  const drawn = $derived.by(() => {
    if (points.length <= MAX_POINTS) return points;
    const size = Math.ceil(points.length / (MAX_POINTS / 2));
    const out: Point[] = [];
    for (let i = 0; i < points.length; i += size) {
      const bucket = points.slice(i, i + size).filter((p) => p[1] !== null) as [number, number][];
      if (!bucket.length) {
        out.push([points[i][0], null]);
        continue;
      }
      const lo = bucket.reduce((a, b) => (b[1] < a[1] ? b : a));
      const hi = bucket.reduce((a, b) => (b[1] > a[1] ? b : a));
      out.push(...(lo[0] <= hi[0] ? [lo, hi] : [hi, lo]));
    }
    return out;
  });

  const path = $derived(
    drawn
      .map(([m, v], i) => (v === null ? "" : `${i && drawn[i - 1][1] !== null ? "L" : "M"}${x(m).toFixed(1)} ${y(v).toFixed(1)}`))
      .join(""),
  );

  // The reading nearest the cursor.
  const hovered = $derived.by(() => {
    if (cursor === null || !points.length) return null;
    let best = points[0];
    for (const p of points) if (Math.abs(p[0] - cursor) < Math.abs(best[0] - cursor)) best = p;
    return best[1];
  });

  const show = (v: number | null) => (v === null ? "--" : `${v.toFixed(digits)}${unit}`);

  function move(e: PointerEvent) {
    const rect = (e.currentTarget as SVGElement).getBoundingClientRect();
    cursor = Math.max(0, Math.min(span, ((e.clientX - rect.left) / rect.width) * span));
  }
</script>

<div class="rounded-xl border border-frame bg-panel p-3">
  <div class="flex items-baseline justify-between">
    <span class="text-xs tracking-widest text-dim uppercase">{label}</span>
    <span class="font-display text-xl font-semibold tabular-nums" style:color>
      {cursor === null ? `${show(low)} – ${show(high)}` : show(hovered)}
    </span>
  </div>
  <svg
    viewBox="0 0 {W} {H}"
    preserveAspectRatio="none"
    class="mt-1 h-28 w-full touch-none"
    role="img"
    aria-label="{label} over the session"
    onpointermove={move}
    onpointerdown={move}
    onpointerleave={() => (cursor = null)}
  >
    {#each [0.25, 0.5, 0.75] as f (f)}
      <line x1="0" x2={W} y1={H * f} y2={H * f} class="stroke-frame" vector-effect="non-scaling-stroke" />
    {/each}
    <path d={path} fill="none" stroke={color} stroke-width="2" vector-effect="non-scaling-stroke" />
    {#if cursor !== null}
      <line x1={x(cursor)} x2={x(cursor)} y1="0" y2={H} class="stroke-dim" vector-effect="non-scaling-stroke" />
    {/if}
  </svg>
</div>
