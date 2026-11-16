// Mounts the wiring page (App.svelte), bundled by Bun with its Svelte plugin (bunfig.toml).
import { mount } from "svelte";
import App from "./App.svelte";

const target = document.getElementById("app");
if (!target) throw new Error("#app is missing from the page");
mount(App, { target });
