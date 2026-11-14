// Captures docs/wiring.html, in color and as a blueprint, into the
// pictures docs/wiring.md shows. `mise run wiring` runs it.
import { join } from "node:path";
import { chromium } from "playwright";

const docs = join(import.meta.dir, "..", "docs");
const page = `file://${join(docs, "wiring.html")}`;

const browser = await chromium.launch();
const tab = await browser.newPage({ viewport: { width: 1300, height: 1000 } });
for (const style of ["color", "blueprint"]) {
  await tab.goto(`${page}#${style}`);
  const output = join(docs, "resources", `wiring-${style}.png`);
  await tab.locator("#diagram").screenshot({ path: output });
  console.log(`wrote ${output}`);
}
await browser.close();
