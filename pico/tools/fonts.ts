// Turns Barlow Condensed (the Android app's font, its TTFs in
// android/app/src/main/res/font) into the Pico's fonts, pico/src/app/ui/fonts.c:
// each glyph's coverage of its pixels, 4 bits a pixel (16 shades: the text
// anti-aliased against whatever it's drawn over), at the sizes font.h
// names. The two biggest only hold the characters they show, for flash.
// The degree sign goes in at 127 (DEL): gfx_text draws it for '\xb0'.
// Run by `mise run fonts`.

import opentype from "opentype.js";

const FONT_DIR = "android/app/src/main/res/font";
const OUT = "pico/src/app/ui/fonts.c";
const ASCII = Array.from({ length: 95 }, (_, i) => String.fromCharCode(32 + i)).join("");

type Spec = { name: string; weight: "medium" | "semibold" | "bold"; size: number; chars?: string };

// size is the em in pixels; font.h says what each is for.
const FONTS: Spec[] = [
  { name: "font_small", weight: "medium", size: 18 },
  { name: "font_label", weight: "semibold", size: 20 },
  { name: "font_text", weight: "semibold", size: 22 },
  { name: "font_value", weight: "bold", size: 34 },
  { name: "font_large", weight: "bold", size: 50 },
  { name: "font_big", weight: "bold", size: 72, chars: " 0123456789.-%CV°" },
  { name: "font_huge", weight: "bold", size: 104, chars: " 0123456789-NSHIFT°" },
];

// Sub-scanlines per pixel row: the vertical anti-aliasing (horizontally,
// each span's ends are exact).
const SUB = 16;

type Edge = { x0: number; y0: number; x1: number; y1: number };
type Glyph = { width: number; height: number; advance: number; xOffset: number; yOffset: number; data: number[] };

// The outline as straight edges, its curves cut in short ones.
function edges(path: opentype.Path): Edge[] {
  const out: Edge[] = [];
  let x = 0;
  let y = 0;
  let startX = 0;
  let startY = 0;
  const line = (nx: number, ny: number) => {
    out.push({ x0: x, y0: y, x1: nx, y1: ny });
    x = nx;
    y = ny;
  };
  for (const c of path.commands) {
    if (c.type === "M") {
      x = startX = c.x;
      y = startY = c.y;
    } else if (c.type === "L") {
      line(c.x, c.y);
    } else if (c.type === "Q") {
      const [ax, ay] = [x, y];
      for (let i = 1; i <= 8; i++) {
        const t = i / 8;
        const u = 1 - t;
        line(u * u * ax + 2 * u * t * c.x1 + t * t * c.x, u * u * ay + 2 * u * t * c.y1 + t * t * c.y);
      }
    } else if (c.type === "C") {
      const [ax, ay] = [x, y];
      for (let i = 1; i <= 12; i++) {
        const t = i / 12;
        const u = 1 - t;
        line(
          u * u * u * ax + 3 * u * u * t * c.x1 + 3 * u * t * t * c.x2 + t * t * t * c.x,
          u * u * u * ay + 3 * u * u * t * c.y1 + 3 * u * t * t * c.y2 + t * t * t * c.y,
        );
      }
    } else if (c.type === "Z") {
      line(startX, startY);
    }
  }
  return out.filter((e) => e.y0 !== e.y1);
}

// The glyph's coverage, nonzero winding, trimmed to its inked pixels.
function rasterize(font: opentype.Font, ch: string, size: number): Glyph {
  const glyph = font.charToGlyph(ch);
  const advance = Math.round(((glyph.advanceWidth ?? 0) * size) / font.unitsPerEm);
  const es = edges(glyph.getPath(0, 0, size));
  if (es.length === 0) return { width: 0, height: 0, advance, xOffset: 0, yOffset: 0, data: [] };
  const xs = es.flatMap((e) => [e.x0, e.x1]);
  const ys = es.flatMap((e) => [e.y0, e.y1]);
  const left = Math.floor(Math.min(...xs));
  const top = Math.floor(Math.min(...ys));
  const w = Math.ceil(Math.max(...xs)) - left;
  const h = Math.ceil(Math.max(...ys)) - top;
  const cover = new Float32Array(w * h);
  for (let row = 0; row < h; row++) {
    for (let s = 0; s < SUB; s++) {
      const sy = top + row + (s + 0.5) / SUB;
      const crossings: { x: number; dir: number }[] = [];
      for (const e of es) {
        const lo = Math.min(e.y0, e.y1);
        const hi = Math.max(e.y0, e.y1);
        if (sy < lo || sy >= hi) continue;
        crossings.push({ x: e.x0 + ((sy - e.y0) / (e.y1 - e.y0)) * (e.x1 - e.x0) - left, dir: e.y1 > e.y0 ? 1 : -1 });
      }
      crossings.sort((a, b) => a.x - b.x);
      let winding = 0;
      for (let i = 0; i < crossings.length - 1; i++) {
        winding += crossings[i].dir;
        if (winding === 0) continue;
        const a = crossings[i].x;
        const b = crossings[i + 1].x;
        for (let px = Math.max(0, Math.floor(a)); px < Math.min(w, Math.ceil(b)); px++) {
          cover[row * w + px] += (Math.min(b, px + 1) - Math.max(a, px)) / SUB;
        }
      }
    }
  }
  const shade = Array.from(cover, (c) => Math.round(Math.min(1, c) * 15));
  // Trimmed to the inked pixels.
  let [x0, y0, x1, y1] = [w, h, -1, -1];
  shade.forEach((v, i) => {
    if (!v) return;
    const x = i % w;
    const y = Math.floor(i / w);
    x0 = Math.min(x0, x);
    y0 = Math.min(y0, y);
    x1 = Math.max(x1, x);
    y1 = Math.max(y1, y);
  });
  if (x1 < 0) return { width: 0, height: 0, advance, xOffset: 0, yOffset: 0, data: [] };
  const data: number[] = [];
  for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) data.push(shade[y * w + x]);
  return { width: x1 - x0 + 1, height: y1 - y0 + 1, advance, xOffset: left + x0, yOffset: top + y0, data };
}

const files = new Map<string, opentype.Font>();
async function load(weight: string): Promise<opentype.Font> {
  const path = `${FONT_DIR}/barlow_condensed_${weight}.ttf`;
  let font = files.get(path);
  if (!font) {
    font = opentype.parse(await Bun.file(path).arrayBuffer());
    files.set(path, font);
  }
  return font;
}

const hex = (bytes: number[]) =>
  Array.from({ length: Math.ceil(bytes.length / 16) }, (_, i) =>
    `    ${bytes
      .slice(i * 16, i * 16 + 16)
      .map((b) => `0x${b.toString(16).padStart(2, "0")},`)
      .join(" ")}`,
  ).join("\n");

let c = `// Generated by \`mise run fonts\` (pico/tools/fonts.ts) from Barlow Condensed
// (SIL Open Font License 1.1, android/app/src/main/assets/OFL.txt): each
// glyph's pixels row after row, 4 bits each (its coverage, 0 to 15), two to
// a byte, high nibble first. 127 is the degree sign.

#include "font.h"
`;
for (const spec of FONTS) {
  const font = await load(spec.weight);
  const scale = spec.size / font.unitsPerEm;
  const chars = spec.chars ?? `${ASCII}°`;
  const bitmap: number[] = [];
  const glyphs: string[] = [];
  for (let code = 32; code <= 127; code++) {
    const ch = code === 127 ? "°" : String.fromCharCode(code);
    const g = chars.includes(ch) ? rasterize(font, ch, spec.size) : null;
    const offset = bitmap.length;
    if (g) {
      for (let i = 0; i < g.data.length; i += 2) bitmap.push((g.data[i] << 4) | (g.data[i + 1] ?? 0));
    }
    const label = ch === "'" ? "\\'" : ch === "\\" ? "\\\\" : ch;
    glyphs.push(
      g
        ? `    {${offset}, ${g.width}, ${g.height}, ${g.advance}, ${g.xOffset}, ${g.yOffset}}, // '${label}'`
        : `    {0, 0, 0, 0, 0, 0},`,
    );
  }
  if (bitmap.length > 0xffff) throw new Error(`${spec.name}: ${bitmap.length} bytes, past glyph_t's 16-bit offsets`);
  const lineHeight = Math.round((font.ascender - font.descender) * scale);
  const zero = rasterize(font, "0", spec.size);
  c += `
// Barlow Condensed ${spec.weight}, ${spec.size}px: ${bitmap.length} bytes.
static const uint8_t ${spec.name}_bitmap[] = {
${hex(bitmap)}
};

static const glyph_t ${spec.name}_glyphs[] = {
${glyphs.join("\n")}
};

const font_t ${spec.name} = {${spec.name}_bitmap, ${spec.name}_glyphs, 32, 127, ${lineHeight}, ${zero.height}};
`;
  console.log(`${spec.name}: ${spec.weight} ${spec.size}px, ${bitmap.length} bytes`);
}
await Bun.write(OUT, c);
