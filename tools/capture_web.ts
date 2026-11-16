// Captures the log page for the README, from made-up drives (none of the
// car's own logs): a session's charts in the dark theme, the list of
// sessions in the light one. `mise run web-pictures` runs it.
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { chromium } from "playwright";
import { serveLog } from "../web/server";

const RECORD = 32;
const resources = join(import.meta.dir, "..", "docs", "resources");

// A lap: [seconds, RPM at its start, at its end, AFR]: pulls through the
// gears (shifts drop the RPM), lifts into corners (the fuel cut reads
// lean), part throttle through them.
const LAP: [number, number, number, number][] = [
  [5, 4200, 7200, 12.3],
  [6, 5300, 7300, 12.2],
  [3, 7300, 4100, 20.2],
  [6, 4100, 4600, 14.6],
  [7, 4600, 7100, 12.4],
  [3, 7100, 3900, 20.1],
  [8, 3900, 4300, 14.7],
  [6, 4300, 7200, 12.3],
  [7, 5200, 7400, 12.2],
  [4, 7400, 3700, 20.2],
  [20, 3700, 3300, 14.8],
];
const LAP_S = LAP.reduce((sum, [s]) => sum + s, 0);

/** RPM and AFR at s seconds into a lap. */
function onLap(s: number): [number, number] {
  let at = s % LAP_S;
  for (const [length, from, to, afr] of LAP) {
    if (at < length) return [from + ((to - from) * at) / length, afr];
    at -= length;
  }
  return [3300, 14.7];
}

/** A made-up drive: minutes long, warming up (gently driven), then laps when on track. */
function drive(session: number, minutes: number, track: boolean): Uint8Array<ArrayBuffer> {
  const count = Math.round((minutes * 60000) / 200);
  const bytes = new Uint8Array(count * RECORD);
  const view = new DataView(bytes.buffer);
  const trackFrom = 180;
  for (let i = 0; i < count; i++) {
    const s = (i * 200) / 1000;
    const warm = Math.min(1, s / 420);
    const lapping = track && s > trackFrom;
    const heat = lapping ? Math.min(1, (s - trackFrom) / 500) : 0;
    const [rpm, afr] = lapping ? onLap(s - trackFrom) : [1600 + Math.sin(s / 9) * 600 + Math.sin(s / 2.3) * 150, 14.7];
    // Flat out on a pull, shut on a lift (the fuel cut), part throttle otherwise.
    const throttle = afr < 13 ? 100 : afr > 19 ? 14 : lapping ? 35 : 18 + Math.max(0, Math.sin(s / 9)) * 15;
    const r = {
      rpm,
      coolant: 40 + warm * 46 + heat * 8,
      oil: 38 + warm * 52 + heat * 26,
      intake: 23 + warm * 3 + heat * 9,
      afr: afr + Math.sin(s * 5) * 0.08,
      battery: 14.15 + Math.sin(s / 31) * 0.04,
    };
    const at = i * RECORD;
    view.setUint16(at, session, true);
    view.setUint8(at + 2, 1);
    view.setUint32(at + 4, i * 200, true);
    const columns = [
      r.rpm,
      r.coolant * 10,
      r.oil * 10,
      r.intake * 10,
      r.afr * 100,
      r.battery * 100,
      660,
      throttle * 10,
    ];
    for (const [k, value] of columns.entries()) view.setInt16(at + 8 + 2 * k, Math.round(value), true);
  }
  return bytes;
}

const dir = mkdtempSync(join(tmpdir(), "dash-log-pictures-"));
const server = serveLog(0, dir);
for (const [session, minutes, track] of [
  [11, 22, false],
  [12, 16, true],
  [13, 6, false],
] as const) {
  await fetch(new URL("/api/sessions", server.url), { method: "POST", body: drive(session, minutes, track) });
}

const browser = await chromium.launch();
async function capture(theme: string, hash: string, name: string, hover = false) {
  const tab = await browser.newPage({ viewport: { width: 1400, height: 900 }, deviceScaleFactor: 1 });
  await tab.addInitScript((mode) => localStorage.setItem("theme", mode), theme);
  await tab.goto(new URL(`/${hash}`, server.url).href);
  await tab.waitForSelector(hash.includes("/logs/") ? "svg[role=img]" : "a[href^='#/logs/']");
  if (hover) {
    const chart = tab.locator("svg[role=img]").first();
    const box = await chart.boundingBox();
    if (box) await tab.mouse.move(box.x + box.width * 0.62, box.y + box.height / 2);
  }
  const output = join(resources, `${name}.png`);
  await tab.screenshot({ path: output });
  console.log(`wrote ${output}`);
  await tab.close();
}
await capture("dark", "#/logs/12", "web-session", true);
await capture("light", "#/logs", "web-logs");
await browser.close();
server.stop();
rmSync(dir, { recursive: true });
