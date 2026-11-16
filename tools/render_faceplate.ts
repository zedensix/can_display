// Renders each printable part of the faceplate's model (the faceplate, the
// tray) to faceplate/stl/<part>.stl, pictures of them to
// faceplate/renders/*.png, the assembly with every module in place to
// faceplate/renders/assembly.scad, and the dimensions they're built from
// to faceplate/data/dimensions.json, which the wiring page's Dimensions
// tab shows.
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
import { assembly, faceplate, parts, tray, vars } from "../faceplate/model";
import { checked, config } from "../faceplate/model/config";

const rendersDir = join("faceplate", "renders");

/**
 * Each picture: what it shows, the camera's rotation [x, y, z] in degrees
 * (OpenSCAD fits the distance), and whether to keep colors (preview mode)
 * rather than render the mesh.
 */
const pictures: Record<string, { object: ScadObject; rotation: number[]; colors?: boolean }> = {
  "faceplate-front": { object: parts.faceplate.rotate([0, 180, 0]), rotation: [25, 0, 0] }, // turned over: what shows in the car
  "faceplate-back": { object: parts.faceplate, rotation: [55, 0, 25] }, // as it sits on the bed: pockets and posts
  "tray-back": { object: parts.tray, rotation: [55, 0, 25] }, // as it sits on the bed: the boards' standoffs
  assembly: { object: assembly, rotation: [70, 0, 140], colors: true }, // from behind, every module in place
  "assembly-front": { object: assembly.rotate([0, 180, 0]), rotation: [55, 0, 20], colors: true }, // what shows in the car
  "assembly-side": { object: assembly, rotation: [90, 0, 90], colors: true }, // from the side: the stack, the front at the bottom
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
    // Each board's pilot hole, from its holes' size.
    pilots: Object.fromEntries(Object.entries(config.boards).map(([name, board]) => [name, tray.pilotOf(board)])),
  },
};
await Bun.write(join("faceplate", "data", "dimensions.json"), `${JSON.stringify(dimensions, null, 2)}\n`);

// The assembly as OpenSCAD source, to turn around in its window (`mise run assembly`).
await Bun.write(
  join(rendersDir, "assembly.scad"),
  `// Written by tools/render_faceplate.ts (mise run faceplate): don't edit. Preview (F5) keeps the colors.\n${assembly.serialize(vars)}`,
);

for (const [name, part] of Object.entries(parts)) {
  console.log(`faceplate: rendering ${name}.stl...`);
  await renderStl(part.serialize(vars), join("faceplate", "stl", `${name}.stl`));
}

const tempDir = await mkdtemp(join(tmpdir(), "faceplate-"));
try {
  for (const [name, picture] of Object.entries(pictures)) {
    console.log(`faceplate: rendering ${name}.png...`);
    const scadPath = join(tempDir, `${name}.scad`);
    await writeFile(scadPath, picture.object.serialize(vars));
    await renderPicture(scadPath, picture.rotation, picture.colors ?? false, join(rendersDir, `${name}.png`));
  }
} finally {
  await rm(tempDir, { recursive: true, force: true });
}
