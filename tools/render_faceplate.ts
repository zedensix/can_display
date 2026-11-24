// Renders the bar's printable parts to faceplate/bar/stl/<part>.stl,
// pictures of them to faceplate/bar/renders/*.png, the bar with every
// module in place to faceplate/bar/renders/bar.scad, and the dimensions
// they're built from to faceplate/data/dimensions.json, which the wiring
// page's Dimensions tab shows.
//
// The STLs come from OpenSCAD compiled to WebAssembly (openscad-wasm),
// in-process. The pictures need the native openscad, which mise installs
// (mise.toml): the WebAssembly build has no GL context to draw with.
// Run through `mise run faceplate`.

import { mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { createOpenSCAD } from "openscad-wasm";
import type { ScadObject } from "scad-js";
import { union } from "scad-js";
import { bar, barBase, front, parts, vars } from "../faceplate";
import { checked, config } from "../faceplate/shared/config";

const OUT = join("faceplate", "bar");

type Picture = { object: ScadObject; rotation: number[]; colors?: boolean };

/**
 * Each picture: what it shows, the camera's rotation [x, y, z] in degrees
 * (OpenSCAD fits the distance), and whether to keep colors (preview mode)
 * rather than render the mesh.
 */
const pictures: Record<string, Picture> = {
  "bar-front": { object: bar.assembly.rotate([0, 180, 0]), rotation: [70, 0, 20], colors: true }, // what shows in the car
  "bar-inside": { object: bar.inside, rotation: [40, 0, 20], colors: true }, // the lid off: every module in place
  "bar-shell-back": { object: bar.shell, rotation: [55, 0, 25] }, // as it sits on the bed
  "bar-lid": { object: parts["bar-lid"], rotation: [55, 0, 25] }, // as it sits on the bed: the skirt, the Pico's standoffs
  "bar-base": { object: parts["bar-base"], rotation: [55, 0, 25] }, // as it sits on the bed, standing on its front edge
  // Clipped into its base, what shows in the car: a hook up each end, over the top.
  "bar-on-base": {
    object: union(bar.assembly, barBase.object.color("tomato")).rotate([0, 180, 0]),
    rotation: [70, 0, 20],
    colors: true,
  },
};

async function renderStl(source: string, path: string): Promise<void> {
  const openscad = (await createOpenSCAD()).getInstance();
  openscad.FS.writeFile("/part.scad", source);
  // Binary STL: a fifth of the ASCII one's size.
  const exitCode = openscad.callMain(["/part.scad", "--export-format", "binstl", "-o", "/part.stl"]);
  if (exitCode !== 0) throw new Error(`openscad-wasm exited with ${exitCode} on ${path}`);
  await Bun.write(path, openscad.FS.readFile("/part.stl"));
}

async function renderPicture(scadPath: string, rotation: number[], colors: boolean, path: string): Promise<void> {
  const openscad = Bun.which("openscad");
  if (!openscad) throw new Error("openscad isn't on PATH: run this through `mise run faceplate`");
  const process = Bun.spawn(
    [
      openscad,
      "--autocenter",
      "--viewall",
      `--camera=0,0,0,${rotation.join(",")},0`,
      ...(colors ? [] : ["--render"]),
      "--projection",
      "perspective",
      "--imgsize",
      "1024,768",
      "-o",
      path,
      scadPath,
    ],
    { stdout: "inherit", stderr: "inherit" },
  );
  if ((await process.exited) !== 0) throw new Error(`openscad failed on ${path}`);
}

const { enclosure: e } = config;
const dimensions = {
  config,
  checked, // the measured keys, from measured.json
  printed: {
    frontThickness: front.thickness,
    // The encoder board's ends from the shaft, its pins near the right one.
    encoderBoard: [front.encoderBoardLeft, front.encoderBoardRight],
    // The bar's outside: length, height at the ends (over the arc), depth.
    bar: [2 * (bar.right + e.wall), 2 * (bar.halfY + e.wall) + bar.sag, bar.back],
  },
};
await Bun.write(join("faceplate", "data", "dimensions.json"), `${JSON.stringify(dimensions, null, 2)}\n`);

// The bar as OpenSCAD source, to turn around in its window (`mise run bar`).
await Bun.write(
  join(OUT, "renders", "bar.scad"),
  `// Written by tools/render_faceplate.ts (mise run faceplate): don't edit. Preview (F5) keeps the colors.\n${bar.assembly.serialize(vars)}`,
);

for (const [name, part] of Object.entries(parts)) {
  console.log(`faceplate: rendering ${name}.stl...`);
  await renderStl(part.serialize(vars), join(OUT, "stl", `${name}.stl`));
}

const tempDir = await mkdtemp(join(tmpdir(), "faceplate-"));
try {
  for (const [name, picture] of Object.entries(pictures)) {
    console.log(`faceplate: rendering ${name}.png...`);
    const scadPath = join(tempDir, `${name}.scad`);
    await writeFile(scadPath, picture.object.serialize(vars));
    await renderPicture(scadPath, picture.rotation, picture.colors ?? false, join(OUT, "renders", `${name}.png`));
  }
} finally {
  await rm(tempDir, { recursive: true, force: true });
}
