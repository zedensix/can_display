// For tsc: a .svelte file is a component (Bun's Svelte plugin compiles
// it), an image an asset's URL (Bun's bundler).
declare module "*.svelte" {
  import type { Component } from "svelte";

  const component: Component<Record<string, unknown>>;
  export default component;
}

declare module "*.png" {
  const url: string;
  export default url;
}
