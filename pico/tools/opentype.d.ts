// What pico/tools/fonts.ts uses of opentype.js, which ships no types.
declare module "opentype.js" {
  namespace opentype {
    type PathCommand =
      | { type: "M" | "L"; x: number; y: number }
      | { type: "Q"; x1: number; y1: number; x: number; y: number }
      | { type: "C"; x1: number; y1: number; x2: number; y2: number; x: number; y: number }
      | { type: "Z" };
    interface Path {
      commands: PathCommand[];
    }
    interface Glyph {
      advanceWidth?: number;
      getPath(x: number, y: number, fontSize: number): Path;
    }
    interface Font {
      unitsPerEm: number;
      ascender: number;
      descender: number;
      charToGlyph(ch: string): Glyph;
    }
    function parse(buffer: ArrayBuffer): Font;
  }
  export default opentype;
}
