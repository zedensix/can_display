// Command logo converts a PNG (dark logo on a light or transparent
// background) into the 1-bit bitmap internal/ui embeds for the boot
// splash: a big-endian uint16 width and height, then rows of bits,
// most significant bit first, each row padded to a whole byte.
//
//	go run ./cmd/logo 86.png internal/ui/logo.bin
package main

import (
	"encoding/binary"
	"fmt"
	"image"
	_ "image/png"
	"os"
)

const size = 200

func main() {
	if len(os.Args) != 3 {
		fmt.Fprintln(os.Stderr, "usage: logo <in.png> <out.bin>")
		os.Exit(2)
	}
	if err := convert(os.Args[1], os.Args[2]); err != nil {
		fmt.Fprintln(os.Stderr, "logo:", err)
		os.Exit(1)
	}
}

func convert(inPath, outPath string) error {
	in, err := os.Open(inPath)
	if err != nil {
		return err
	}
	defer in.Close()
	img, _, err := image.Decode(in)
	if err != nil {
		return err
	}

	bounds := img.Bounds()
	width := size
	height := size * bounds.Dy() / bounds.Dx()
	rowBytes := (width + 7) / 8

	out := make([]byte, 4, 4+rowBytes*height)
	binary.BigEndian.PutUint16(out[0:], uint16(width))
	binary.BigEndian.PutUint16(out[2:], uint16(height))
	for y := range height {
		row := make([]byte, rowBytes)
		for x := range width {
			if inked(img, bounds, x, y, width, height) {
				row[x/8] |= 0x80 >> (x % 8)
			}
		}
		out = append(out, row...)
	}
	return os.WriteFile(outPath, out, 0o644)
}

// inked averages the source pixels that land on target pixel (x, y)
// and reports whether they are mostly dark and opaque.
func inked(img image.Image, bounds image.Rectangle, x, y, width, height int) bool {
	x0 := bounds.Min.X + x*bounds.Dx()/width
	x1 := bounds.Min.X + (x+1)*bounds.Dx()/width
	y0 := bounds.Min.Y + y*bounds.Dy()/height
	y1 := bounds.Min.Y + (y+1)*bounds.Dy()/height
	var ink, count uint64
	for sy := y0; sy < y1; sy++ {
		for sx := x0; sx < x1; sx++ {
			r, g, b, a := img.At(sx, sy).RGBA()
			luminance := (299*uint64(r) + 587*uint64(g) + 114*uint64(b)) / 1000
			ink += uint64(a) - luminance*uint64(a)/0xffff
			count++
		}
	}
	return count > 0 && ink/count > 0x7fff
}
