"""Turn a PNG into the boot splash: 200px wide, 1 bit per pixel.

Output: big-endian uint16 width and height, then rows MSB first, each
padded to a byte. A pixel is inked when the source pixels it covers are,
on average, more dark than light (alpha-weighted). Stdlib only, so it
decodes 8-bit non-interlaced PNGs itself.
"""

import struct
import sys
import zlib

WIDTH = 200
CHANNELS = {0: 1, 2: 3, 4: 2, 6: 4}  # PNG color type -> samples per pixel


def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit(f"{path}: not a PNG")
    pos, idat = 8, b""
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos : pos + 8])
        body = data[pos + 8 : pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    if depth != 8 or interlace or color not in CHANNELS:
        sys.exit(f"{path}: only 8-bit, non-interlaced gray/RGB(A) PNGs are supported")
    channels = CHANNELS[color]
    raw = zlib.decompress(idat)
    stride = width * channels
    rows, prev = [], bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        kind, row = raw[start], bytearray(raw[start + 1 : start + 1 + stride])
        for i in range(stride):
            left = row[i - channels] if i >= channels else 0
            up = prev[i]
            up_left = prev[i - channels] if i >= channels else 0
            if kind == 1:
                row[i] = (row[i] + left) & 0xFF
            elif kind == 2:
                row[i] = (row[i] + up) & 0xFF
            elif kind == 3:
                row[i] = (row[i] + (left + up) // 2) & 0xFF
            elif kind == 4:
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                pred = left if pa <= pb and pa <= pc else up if pb <= pc else up_left
                row[i] = (row[i] + pred) & 0xFF
        rows.append(row)
        prev = row
    return width, height, channels, rows


def darkness(pixel, channels):
    """How much ink a pixel holds, 0 to 1."""
    if channels <= 2:
        luminance, alpha = pixel[0], pixel[1] if channels == 2 else 255
    else:
        luminance = (299 * pixel[0] + 587 * pixel[1] + 114 * pixel[2]) / 1000
        alpha = pixel[3] if channels == 4 else 255
    return alpha / 255 * (1 - luminance / 255)


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: logo.py <in.png> <out.bin>")
    src_w, src_h, channels, rows = read_png(sys.argv[1])
    height = WIDTH * src_h // src_w
    out = bytearray(struct.pack(">HH", WIDTH, height))
    for y in range(height):
        bits = bytearray((WIDTH + 7) // 8)
        y0, y1 = y * src_h // height, (y + 1) * src_h // height
        for x in range(WIDTH):
            x0, x1 = x * src_w // WIDTH, (x + 1) * src_w // WIDTH
            ink = [
                darkness(rows[sy][sx * channels : (sx + 1) * channels], channels)
                for sy in range(y0, y1)
                for sx in range(x0, x1)
            ]
            if ink and sum(ink) / len(ink) > 0.5:
                bits[x // 8] |= 0x80 >> (x % 8)
        out += bits
    open(sys.argv[2], "wb").write(out)


main()
