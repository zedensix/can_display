// `bun run wiring`: serves docs/wiring.html and the faceplate's files on
// localhost and opens it. Its Dimensions tab saves each dimension as you
// type it through POST /measurements: it goes to faceplate/data/measured.json
// at once, then `mise run faceplate` rebuilds the parts. If the model
// refuses the values (a standoff off the tray, a hole in a pocket...),
// they stay saved and the page shows why; GET /measurements reads them back.

import { join, resolve, sep } from "node:path";
import { config, numberAt } from "../faceplate/model/config";

const root = resolve(import.meta.dir, "..");
const measuredPath = join(root, "faceplate", "data", "measured.json");
// Only these folders are served.
const served = ["docs", "faceplate"].map((dir) => join(root, dir) + sep);

/** Checks the page's values: known dimensions, plain non-negative numbers. */
function parseMeasurements(body: unknown): Record<string, number> {
  if (typeof body !== "object" || body === null || Array.isArray(body)) throw new Error("expected an object");
  const values: Record<string, number> = {};
  for (const [key, value] of Object.entries(body)) {
    if (numberAt(config, key) === undefined) throw new Error(`${key} isn't a dimension in faceplate/model/config.ts`);
    if (typeof value !== "number" || !Number.isFinite(value) || value < 0)
      throw new Error(`${key}: ${value} isn't a dimension`);
    values[key] = value;
  }
  return values;
}

// The last rebuild's refusal (the model's own message), null when it
// built, and the measured.json it was built from: once the file changes
// (a fix, a rebuild from the terminal), the refusal no longer applies.
let lastError: string | null = null;
let lastBuiltFrom = "";
let building: Promise<string | null> | null = null;
let stale = false;

/** Runs `mise run faceplate`: null when it built, else why the model refuses. */
async function runFaceplate(): Promise<string | null> {
  const build = Bun.spawn(["mise", "run", "faceplate"], { cwd: root, stdout: "pipe", stderr: "pipe" });
  const [output, errors, exitCode] = await Promise.all([
    new Response(build.stdout).text(),
    new Response(build.stderr).text(),
    build.exited,
  ]);
  if (exitCode === 0) return null;
  // The model's own message ("boards.pico: a standoff falls off the tray...") is on an "error:" line.
  const reason = `${output}\n${errors}`.split("\n").find((line) => line.startsWith("error:"));
  return (reason ?? errors.split("\n").slice(-15).join("\n")).replace(/^error: /, "");
}

/**
 * Rebuilds from measured.json. Saves arriving during a build share it and
 * get one more pass, so the parts always end up built from the last values.
 */
function rebuild(): Promise<string | null> {
  if (building) {
    stale = true;
    return building;
  }
  building = (async () => {
    do {
      stale = false;
      lastBuiltFrom = await Bun.file(measuredPath).text();
      lastError = await runFaceplate();
    } while (stale);
    return lastError;
  })().finally(() => {
    building = null;
  });
  return building;
}

/**
 * Saves the measurements first, whatever the model makes of them: they're
 * facts about the modules, and a layout they break is the layout's
 * problem. Then rebuilds, and answers with the model's refusal, if any.
 */
async function saveMeasurements(request: Request): Promise<Response> {
  let values: Record<string, number>;
  try {
    values = parseMeasurements(await request.json());
  } catch (error) {
    return new Response(String(error), { status: 400 });
  }
  await Bun.write(measuredPath, `${JSON.stringify(values, null, 2)}\n`);
  return Response.json({ saved: true, error: await rebuild() });
}

/** The saved measurements, newer than the page's dimensions.js when a rebuild was refused. */
async function readMeasurements(): Promise<Response> {
  const saved = await Bun.file(measuredPath).text();
  return Response.json({ values: JSON.parse(saved), error: saved === lastBuiltFrom ? lastError : null });
}

const server = Bun.serve({
  port: Number(process.env.PORT ?? 3001),
  routes: {
    "/": Response.redirect("/docs/wiring.html"),
    "/measurements": { GET: readMeasurements, POST: saveMeasurements },
  },
  async fetch(request) {
    const path = resolve(root, `.${decodeURIComponent(new URL(request.url).pathname)}`);
    const file = Bun.file(path);
    if (!served.some((dir) => path.startsWith(dir)) || !(await file.exists())) {
      return new Response("not found", { status: 404 });
    }
    return new Response(file, { headers: { "cache-control": "no-store" } });
  },
});

const url = new URL("/docs/wiring.html", server.url).href;
console.log(`wiring and dimensions on ${url} (ctrl+c to stop)`);
// Once per process: `bun --hot` (bun run dev:wiring) reruns this file on
// every change, measured.json included, but keeps globalThis.
const shared = globalThis as { wiringPageOpened?: boolean };
if (!shared.wiringPageOpened) {
  shared.wiringPageOpened = true;
  Bun.spawn(["open", url]);
}
