"""Convert binary PPM (P6) files to PNG, with the standard library only.

Usage: python3 tools/ppm2png.py <out dir> <file.ppm>...
"""

import os
import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as f:
        data = f.read()
    magic, width, height, maxval, pixels = data.split(maxsplit=4)
    if magic != b"P6" or maxval != b"255":
        raise ValueError(f"{path}: not an 8-bit P6 PPM")
    return int(width), int(height), pixels


def chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))


def write_png(path, width, height, pixels):
    stride = width * 3
    raw = b"".join(b"\0" + pixels[y * stride : (y + 1) * stride] for y in range(height))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def main():
    out_dir, paths = sys.argv[1], sys.argv[2:]
    os.makedirs(out_dir, exist_ok=True)
    for path in paths:
        name = os.path.splitext(os.path.basename(path))[0] + ".png"
        write_png(os.path.join(out_dir, name), *read_ppm(path))


if __name__ == "__main__":
    main()
