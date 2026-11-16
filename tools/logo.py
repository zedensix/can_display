"""Turn a PNG into a logo for the panel, 1 bit per pixel: 200px wide, or
fitted into a max_width x max_height box when given.

Output: big-endian uint16 width and height, then rows MSB first, each
padded to a byte. A pixel is inked when the source pixels it covers are,
on average, more logo than background (alpha-weighted): dark ones, or,
for a light logo on a transparent background (its opaque pixels light on
average), light ones. An output pixel covering many source pixels
samples a grid of them, not all: a 5000px logo would take minutes. Stdlib only,
so it decodes non-interlaced PNGs itself: gray, RGB, gray or RGB with
alpha at 8 bits, and palettes (with their transparency) at 1 to 8 bits.
"""

import struct
import sys
import zlib

WIDTH = 200
CHANNELS = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}  # PNG color type -> samples per pixel
GRID = 4  # samples per output pixel, along each side


def unfilter(raw, height, stride, bpp):
    """The image's rows, its scanline filters undone."""
    rows, prev = [], bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        kind, row = raw[start], bytearray(raw[start + 1 : start + 1 + stride])
        if kind == 1:
            for i in range(bpp, stride):
                row[i] = (row[i] + row[i - bpp]) & 0xFF
        elif kind == 2:
            row = bytearray((a + b) & 0xFF for a, b in zip(row, prev))
        elif kind == 3:
            for i in range(stride):
                row[i] = (row[i] + ((row[i - bpp] if i >= bpp else 0) + prev[i]) // 2) & 0xFF
        elif kind == 4:
            for i in range(stride):
                left = row[i - bpp] if i >= bpp else 0
                up, up_left = prev[i], prev[i - bpp] if i >= bpp else 0
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                row[i] = (row[i] + (left if pa <= pb and pa <= pc else up if pb <= pc else up_left)) & 0xFF
        rows.append(row)
        prev = row
    return rows


def read_png(path):
    """The image's size and a function giving a pixel's (luminance, alpha), 0 to 1."""
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit(f"{path}: not a PNG")
    pos, idat, palette, alphas = 8, b"", [], b""
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos : pos + 8])
        body = data[pos + 8 : pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = [tuple(body[i : i + 3]) for i in range(0, len(body), 3)]
        elif kind == b"tRNS":
            alphas = body
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    if interlace or color not in CHANNELS or (depth != 8 and color != 3):
        sys.exit(f"{path}: only non-interlaced 8-bit gray/RGB(A) or 1-8 bit palette PNGs are supported")
    channels = CHANNELS[color]
    stride = (width * channels * depth + 7) // 8
    rows = unfilter(zlib.decompress(idat), height, stride, max(1, channels * depth // 8))

    def luminance(r, g, b):
        return (299 * r + 587 * g + 114 * b) / 1000 / 255

    def pixel(x, y):
        row = rows[y]
        if color == 3:
            bit = x * depth
            index = (row[bit // 8] >> (8 - depth - bit % 8)) & ((1 << depth) - 1)
            alpha = alphas[index] if index < len(alphas) else 255
            return luminance(*palette[index]), alpha / 255
        p = row[x * channels : (x + 1) * channels]
        if channels <= 2:
            return p[0] / 255, (p[1] if channels == 2 else 255) / 255
        return luminance(p[0], p[1], p[2]), (p[3] if channels == 4 else 255) / 255

    return width, height, color, pixel


def main():
    if len(sys.argv) not in (3, 5):
        sys.exit("usage: logo.py <in.png> <out.bin> [max_width max_height]")
    src_w, src_h, color, pixel = read_png(sys.argv[1])
    # A light logo on a transparent background inks its light pixels.
    step = max(1, int((src_w * src_h / 40000) ** 0.5))
    opaque = [lum for y in range(0, src_h, step) for x in range(0, src_w, step) for lum, a in [pixel(x, y)] if a > 0.5]
    transparent = len(opaque) < ((src_w + step - 1) // step) * ((src_h + step - 1) // step)
    light = transparent and opaque and sum(opaque) / len(opaque) > 0.6

    def ink(x, y):
        lum, alpha = pixel(x, y)
        return alpha * (lum if light else 1 - lum)

    # Fitted to the logo, not to its margins: cropped to its inked pixels.
    if len(sys.argv) == 5:
        inked = [(x, y) for y in range(0, src_h, step) for x in range(0, src_w, step) if ink(x, y) > 0.5]
        if inked:
            left, top = max(0, min(x for x, _ in inked) - step), max(0, min(y for _, y in inked) - step)
            right = min(src_w, max(x for x, _ in inked) + 2 * step)
            bottom = min(src_h, max(y for _, y in inked) + 2 * step)
            whole = ink
            src_w, src_h = right - left, bottom - top

            def ink(x, y):
                return whole(x + left, y + top)

    width = WIDTH
    if len(sys.argv) == 5:
        width = min(int(sys.argv[3]), int(sys.argv[4]) * src_w // src_h)
    height = width * src_h // src_w
    out = bytearray(struct.pack(">HH", width, height))
    def covered(i, size, src):
        """The source pixels output pixel i covers, along one side: all of them, or a grid of them when many."""
        start, end = i * src // size, max((i + 1) * src // size, i * src // size + 1)
        if end - start <= GRID:
            return range(start, end)
        return [start + k * (end - start) // GRID for k in range(GRID)]

    for y in range(height):
        bits = bytearray((width + 7) // 8)
        sy = covered(y, height, src_h)
        for x in range(width):
            samples = [ink(a, b) for b in sy for a in covered(x, width, src_w)]
            if sum(samples) / len(samples) > 0.5:
                bits[x // 8] |= 0x80 >> (x % 8)
        out += bits
    open(sys.argv[2], "wb").write(out)


main()
