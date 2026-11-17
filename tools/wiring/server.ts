// `bun run wiring`: serves the wiring page (index.html, Svelte, bundled by
// Bun) on localhost and opens it. Its Dimensions tab reads what the parts
// are built from (GET /dimensions: faceplate/data/dimensions.json, written
// by `mise run faceplate`) and saves each dimension as you type it
// through POST /measurements: it goes to faceplate/data/measured.json at
// once, then `mise run faceplate` rebuilds the parts. If the model
// refuses the values (a standoff off the tray, a hole in a pocket...),
// they stay saved and the page shows why; GET /measurements reads them
// back. GET /renders/<name>.png is a render of the parts.

import { join, resolve } from "node:path";
import { config, numberAt } from "../../faceplate/shared/config";
import page from "./index.html";

const root = resolve(import.meta.dir, "..", "..");
const measuredPath = join(root, "faceplate", "data", "measured.json");
const dimensionsPath = join(root, "faceplate", "data", "dimensions.json");
const rendersDir = join(root, "faceplate", "plate", "renders");
const noStore = { "cache-control": "no-store" };

/** Checks the page's values: known dimensions, plain non-negative numbers. */
function parseMeasurements(body: unknown): Record<string, number> {
  if (typeof body !== "object" || body === null || Array.isArray(body)) throw new Error("expected an object");
  const values: Record<string, number> = {};
  for (const [key, value] of Object.entries(body)) {
    if (numberAt(config, key) === undefined) throw new Error(`${key} isn't a dimension in faceplate/shared/config.ts`);
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

/** The saved measurements, newer than dimensions.json when a rebuild was refused. */
async function readMeasurements(): Promise<Response> {
  const saved = await Bun.file(measuredPath).text();
  return Response.json({ values: JSON.parse(saved), error: saved === lastBuiltFrom ? lastError : null });
}

/** A file, never cached: it changes with every rebuild. */
async function fresh(path: string): Promise<Response> {
  const file = Bun.file(path);
  return (await file.exists()) ? new Response(file, { headers: noStore }) : new Response("not found", { status: 404 });
}

/** Serves the wiring page on port (0: any free one). */
export function serveWiring(port: number) {
  return Bun.serve({
    port,
    routes: {
      "/": page,
      "/dimensions": () => fresh(dimensionsPath),
      "/measurements": { GET: readMeasurements, POST: saveMeasurements },
      // A name, nothing else: no path out of the renders' folder.
      "/renders/:name": (request) =>
        /^[a-z-]+\.png$/.test(request.params.name)
          ? fresh(join(rendersDir, request.params.name))
          : new Response("not found", { status: 404 }),
    },
  });
}

if (import.meta.main) {
  const server = serveWiring(Number(process.env.PORT ?? 3001));
  console.log(`wiring and dimensions on ${server.url} (ctrl+c to stop)`);
  // Once per process: `bun --hot` (bun run dev:wiring) reruns this file on
  // every change, measured.json included, but keeps globalThis.
  const shared = globalThis as { wiringPageOpened?: boolean };
  if (!shared.wiringPageOpened) {
    shared.wiringPageOpened = true;
    Bun.spawn(["open", server.url.href]);
  }
}
