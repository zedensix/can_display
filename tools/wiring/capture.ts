// Captures the wiring diagram, in color and as a blueprint, into the
// pictures docs/wiring.md shows. `mise run wiring` runs it.
import { join } from "node:path";
import { chromium } from "playwright";
import { serveWiring } from "./server";

const resources = join(import.meta.dir, "..", "..", "docs", "resources");
const server = serveWiring(0);
const browser = await chromium.launch();
const tab = await browser.newPage({ viewport: { width: 1300, height: 1000 } });
for (const style of ["color", "blueprint"]) {
  await tab.goto(new URL(`/#${style}`, server.url).href);
  const output = join(resources, `wiring-${style}.png`);
  await tab.locator("#diagram").screenshot({ path: output });
  console.log(`wrote ${output}`);
}
await browser.close();
server.stop();
