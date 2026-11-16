<!-- A module's card: its drawings, and a row per dimension, editable when
     it's a config key. Hovering a row highlights what it measures. -->
<script lang="ts">
  import { type CardSpec, highlightKey, mm } from "./dimensions/drawing";

  let {
    spec,
    checked,
    edits,
    onEdit,
  }: {
    spec: CardSpec;
    checked: Record<string, number>;
    edits: Record<string, number>;
    onEdit: (key: string, value: number) => void;
  } = $props();

  let card: HTMLElement | undefined = $state();
  const invalid: Record<string, boolean> = $state({});
  // The renders as of this card's drawing, not a cached older rebuild.
  const stamp = Date.now();

  // The latest value: a draft not saved yet, else the saved one (newer than
  // the drawings when the model refused it), else the config's.
  const current = (key: string, value: number) => edits[key] ?? checked[key] ?? value;
  const known = (key: string) => key in checked || key in edits;

  function typed(key: string, text: string) {
    const number = Number(text);
    invalid[key] = text === "" || !Number.isFinite(number) || number < 0;
    if (!invalid[key]) onEdit(key, number);
  }

  const highlight = (key: string | null | undefined) => card && highlightKey(card, key ?? null);
</script>

<section class="card" bind:this={card}>
  <h2>{spec.title}</h2>
  <p class="sub">{spec.sub}</p>
  <div class="body">
    {#if spec.drawings.length}
      <div class="views" {@attach (node) => node.append(...spec.drawings)}></div>
    {/if}
    <table>
      <tbody>
        {#each spec.rows as [label, value, key, shows] (label)}
          <tr
            class:edited={key !== null && key in edits}
            onmouseenter={() => highlight(shows ?? key)}
            onmouseleave={() => highlight(null)}
            onfocusin={() => highlight(shows ?? key)}
            onfocusout={() => highlight(null)}
          >
            <td>{label}<span class="key">{key ?? "derived from the others"}</span></td>
            {#if key}
              <td class="value">
                <input
                  type="number"
                  step="any"
                  min="0"
                  defaultValue={current(key, value)}
                  class:invalid={invalid[key]}
                  aria-label="{label} (mm)"
                  oninput={(e) => typed(key, e.currentTarget.value)}
                />
                mm
              </td>
              <td>
                <button
                  type="button"
                  class="chip {known(key) ? 'checked' : 'guess'}"
                  title="Click a guess to confirm its value as right"
                  onclick={() => !known(key) && onEdit(key, current(key, value))}
                >
                  {known(key) ? "checked" : "guess"}
                </button>
              </td>
            {:else}
              <td class="value">{mm(value)}</td>
              <td></td>
            {/if}
          </tr>
        {/each}
      </tbody>
    </table>
  </div>
  {#if spec.pictures}
    <div class="pictures">
      {#each spec.pictures as name (name)}
        <img src="/renders/{name}.png?{stamp}" alt={name} />
      {/each}
    </div>
  {/if}
</section>
