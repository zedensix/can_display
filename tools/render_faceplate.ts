// Renders each model's printable parts (the faceplate and tray, the vent
// pod, the head, the bar) to faceplate/<model>/stl/<part>.stl, pictures of them to
// faceplate/<model>/renders/*.png, each assembly with every module in place
// to faceplate/<model>/renders/<name>.scad, and the dimensions they're built
// from to faceplate/data/dimensions.json, which the wiring page's
// Dimensions tab shows.
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
import { difference } from "scad-js";
import { assembly, bar, faceplate, head, models, onVent, pod, tray, vars, ventMount } from "../faceplate";
import { checked, config } from "../faceplate/shared/config";
import { box } from "../faceplate/shared/geometry";

type Picture = { object: ScadObject; rotation: number[]; colors?: boolean };

/**
 * Each picture: what it shows, the camera's rotation [x, y, z] in degrees
 * (OpenSCAD fits the distance), and whether to keep colors (preview mode)
 * rather than render the mesh.
 */
const pictures: Record<string, Record<string, Picture>> = {
  plate: {
    "faceplate-front": { object: models.plate.faceplate.rotate([0, 180, 0]), rotation: [25, 0, 0] }, // turned over: what shows in the car
    "faceplate-back": { object: models.plate.faceplate, rotation: [55, 0, 25] }, // as it sits on the bed: pockets and posts
    "tray-back": { object: models.plate.tray, rotation: [55, 0, 25] }, // as it sits on the bed: the boards' standoffs
    assembly: { object: assembly, rotation: [70, 0, 140], colors: true }, // from behind, every module in place
    "assembly-front": { object: assembly.rotate([0, 180, 0]), rotation: [55, 0, 20], colors: true }, // what shows in the car
    "assembly-side": { object: assembly, rotation: [90, 0, 90], colors: true }, // from the side: the stack, the front at the bottom
    "vent-mount": { object: onVent, rotation: [90, 0, 90], colors: true }, // on its temporary vent hook, from the side
    "vent-clamp": { object: ventMount.clamp, rotation: [70, 0, 140], colors: true }, // the clamp alone, from behind the dash
  },
  pod: {
    "pod-front": { object: pod.assembly.rotate([0, 180, 0]), rotation: [70, 0, 20], colors: true }, // what shows in the car
    "pod-side": { object: pod.assembly, rotation: [90, 0, 60], colors: true }, // the can cut open: the sled and its boards
    "pod-face-back": { object: pod.face, rotation: [235, 0, 25] }, // the room behind the face, the spigot
    "pod-sled": { object: models.pod["pod-sled"], rotation: [55, 0, 25] }, // as it sits on the bed
  },
  head: {
    "head-front": { object: head.assembly.rotate([0, 180, 0]), rotation: [70, 0, 20], colors: true }, // what shows in the car
    // Cut open across the Pico's standoffs: the stack, display, Pico, lid.
    "head-side": {
      object: difference(head.assembly, box(head.picoHoles[0][0], 50, -50, 50, -50, 50)),
      rotation: [90, 0, 90],
      colors: true,
    },
    "head-shell-back": { object: head.shell, rotation: [55, 0, 25] }, // as it sits on the bed: the room, the bosses, the USB notch
    "head-lid": { object: models.head["head-lid"], rotation: [55, 0, 25] }, // as it sits on the bed: the Pico's standoffs
  },
  bar: {
    "bar-front": { object: bar.assembly.rotate([0, 180, 0]), rotation: [70, 0, 20], colors: true }, // what shows in the car
    "bar-inside": {
      object: bar.inside,
      rotation: [40, 0, 20],
      colors: true,
    }, // the lid off: every module in place
    "bar-shell-back": { object: bar.shell, rotation: [55, 0, 25] }, // as it sits on the bed
    "bar-lid": { object: models.bar["bar-lid"], rotation: [55, 0, 25] }, // as it sits on the bed: the tabs, the Pico's standoffs
    "bar-arcs": { object: models.bar["bar-arcs"], rotation: [0, 0, 0] }, // the test print: a strip for each arc, from the front
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

const dimensions = {
  config,
  checked, // the measured keys, from measured.json
  printed: {
    plateThickness: faceplate.thickness,
    outlineWidth: 2 * faceplate.halfWidth,
    outlineHeight: faceplate.top - faceplate.bottom,
    // The encoder board's ends from the shaft, its pins near the right one.
    encoderBoard: [faceplate.encoderBoardLeft, faceplate.encoderBoardRight],
    depth: faceplate.thickness + config.tray.gap + config.tray.thickness,
    // The head's outside.
    head: [2 * (head.halfX + config.head.wall), 2 * (head.halfY + config.head.wall), head.depth],
    // The bar's outside, its height at the ends, the saddle's arc under them.
    bar: [2 * (bar.right + config.head.wall), 2 * (bar.halfY + config.head.wall) + bar.sag, bar.back],
    // Each board's pilot hole, from its holes' size.
    pilots: Object.fromEntries(Object.entries(config.boards).map(([name, board]) => [name, tray.pilotOf(board)])),
  },
};
await Bun.write(join("faceplate", "data", "dimensions.json"), `${JSON.stringify(dimensions, null, 2)}\n`);

// Each assembly as OpenSCAD source, to turn around in its window (`mise run assembly`, `pod`, `head`, `bar`).
const scads: Record<string, Record<string, ScadObject>> = {
  plate: { assembly },
  pod: { pod: pod.assembly }, // the can cut open on the right
  head: { head: head.assembly },
  bar: { bar: bar.assembly },
};
for (const [model, files] of Object.entries(scads)) {
  for (const [name, object] of Object.entries(files)) {
    await Bun.write(
      join("faceplate", model, "renders", `${name}.scad`),
      `// Written by tools/render_faceplate.ts (mise run faceplate): don't edit. Preview (F5) keeps the colors.\n${object.serialize(vars)}`,
    );
  }
}

for (const [model, parts] of Object.entries(models)) {
  for (const [name, part] of Object.entries(parts)) {
    console.log(`faceplate: rendering ${model}/${name}.stl...`);
    await renderStl(part.serialize(vars), join("faceplate", model, "stl", `${name}.stl`));
  }
}

const tempDir = await mkdtemp(join(tmpdir(), "faceplate-"));
try {
  for (const [model, shots] of Object.entries(pictures)) {
    for (const [name, picture] of Object.entries(shots)) {
      console.log(`faceplate: rendering ${model}/${name}.png...`);
      const scadPath = join(tempDir, `${name}.scad`);
      await writeFile(scadPath, picture.object.serialize(vars));
      const png = join("faceplate", model, "renders", `${name}.png`);
      await renderPicture(scadPath, picture.rotation, picture.colors ?? false, png);
    }
  }
} finally {
  await rm(tempDir, { recursive: true, force: true });
}
