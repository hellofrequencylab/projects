#!/usr/bin/env python3
"""Generate the boot-screen unicorn for cyberpunkunicorn OS.

Original pixel art defined below as character grids (two gallop frames) + a
palette. Running this script deterministically regenerates:

  ../apps/boot/unicorn_sprite.h  palette-indexed frames for the firmware
  unicorn_preview.png  both frames, upscaled, mane coloured as on the device
  boot_preview.png     320x240 mock of one boot-scene frame

'R' pixels are drawn by the firmware in an animated rainbow (hue depends on the
row and on time), so they are stored as a special index, not a colour.

Usage: .venv/bin/python assets/make_unicorn.py   (needs Pillow)
"""

import colorsys
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
SCALE = 3  # device pixels per grid cell

# index 0 = transparent, 1 = rainbow (animated); the rest are fixed colours
PALETTE = {
    ".": (0, None),
    "R": (1, None),
    "K": (2, (24, 6, 40)),       # outline, deep purple
    "W": (3, (255, 255, 255)),   # body
    "w": (4, (200, 180, 255)),   # body shade, lavender
    "H": (5, (255, 220, 60)),    # horn
    "h": (6, (255, 140, 0)),     # horn shade
    "E": (7, (0, 255, 255)),     # cyber eye, neon cyan
    "P": (8, (255, 90, 200)),    # nose / cheek, hot pink
    "G": (9, (0, 255, 160)),     # neon hooves
}

# fmt: off
FRAME_A = [
    "...................................H....",
    "..................................HhK...",
    ".................................HhK....",
    "................................HhK.....",
    ".......................RRRR....KHhK.....",
    ".....................RRRRRRRKKKWWK......",
    "....................RRRRRRRKWWWWWWK.....",
    "...................RRRRRRRKWWWWWEEWK....",
    "..................RRRRRRRKWWWWWWEEWWK...",
    "..................RRRRRRKWWWWWWWWWWWWK..",
    ".................RRRRRRKWWWWWWWWWWWWPWK.",
    ".................RRRRRKWWWWWWKKKWWWWWWK.",
    "................RRRRRKWWWWWWK..KKKKKKK..",
    "................RRRRKWWWWWWK............",
    "..RRRR.........RRRRKWWWWWWWK............",
    ".RRRRRRR...KKKKKKKKWWWWWWWWK............",
    "RRRRRRRRKKKWWWWWWWWWWWWWWWWK............",
    "RRRRRRRKWWWWWWWWWWWWWWWWWWWWK...........",
    ".RRRRRKWWWWWWWWWWWWWWWWWWWWWK...........",
    "..RRRRKWWWWWWWWWWWWWWWWWWWWWK...........",
    "...RRRKwWWWWWWWWWWWWWWWWWWWwK...........",
    "....RRKwwWWWWWWWWWWWWWWWWWwwK...........",
    ".....RKwwwKKwwwwwwwwwwKKwwwK............",
    "......KwwK.KKKKKKKKKKK.KwwK.............",
    ".....KwwK...............KwwK............",
    "....KwwK.................KwwK...........",
    "...KwwK...................KwwK..........",
    "..KGGK.....................KGGK.........",
    "..KKK.......................KKK.........",
]

FRAME_B = [
    "...................................H....",
    "..................................HhK...",
    ".................................HhK....",
    "................................HhK.....",
    ".......................RRRR....KHhK.....",
    ".....................RRRRRRRKKKWWK......",
    "....................RRRRRRRKWWWWWWK.....",
    "...................RRRRRRRKWWWWWEEWK....",
    "..................RRRRRRRKWWWWWWEEWWK...",
    "..................RRRRRRKWWWWWWWWWWWWK..",
    ".................RRRRRRKWWWWWWWWWWWWPWK.",
    ".................RRRRRKWWWWWWKKKWWWWWWK.",
    "................RRRRRKWWWWWWK..KKKKKKK..",
    "................RRRRKWWWWWWK............",
    "...............RRRRKWWWWWWWK............",
    "RRRRRRRR...KKKKKKKKWWWWWWWWK............",
    ".RRRRRRRKKKWWWWWWWWWWWWWWWWK............",
    "..RRRRRKWWWWWWWWWWWWWWWWWWWWK...........",
    "...RRRKWWWWWWWWWWWWWWWWWWWWWK...........",
    "...RRRKWWWWWWWWWWWWWWWWWWWWWK...........",
    "..RRRRKwWWWWWWWWWWWWWWWWWWWwK...........",
    ".RRRR.KwwWWWWWWWWWWWWWWWWWwwK...........",
    "......KwwwKKwwwwwwwwwwKKwwwK............",
    ".......KwwKKKKKKKKKKKKKwwK..............",
    "........KwwK.........KwwK...............",
    ".........KwwK.......KwwK................",
    "..........KwwK.....KwwK.................",
    "..........KGGK.....KGGK.................",
    "..........KKK......KKK..................",
]
# fmt: on

FRAMES = [FRAME_A, FRAME_B]


def check():
    w = len(FRAMES[0][0])
    for f in FRAMES:
        assert len(f) == len(FRAMES[0]), "frames must have the same height"
        for row in f:
            assert len(row) == w, f"row width {len(row)} != {w}: {row!r}"
            for ch in row:
                assert ch in PALETTE, f"unknown pixel {ch!r}"
    return w, len(FRAMES[0])


def rainbow(row, t=0.0):
    # Same formula as the firmware: hue walks down the mane, shifts with time.
    r, g, b = colorsys.hsv_to_rgb(((row * 18 + t) % 360) / 360.0, 1.0, 1.0)
    return int(r * 255), int(g * 255), int(b * 255)


def render(frame, t=0.0, scale=SCALE):
    w, h = len(frame[0]), len(frame)
    img = Image.new("RGBA", (w * scale, h * scale), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    for y, row in enumerate(frame):
        for x, ch in enumerate(row):
            idx, rgb = PALETTE[ch]
            if idx == 0:
                continue
            if idx == 1:
                rgb = rainbow(y, t)
            d.rectangle([x * scale, y * scale, x * scale + scale - 1, y * scale + scale - 1], fill=rgb)
    return img


def rgb565(rgb):
    r, g, b = rgb
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def write_header(w, h):
    out = [
        "// Generated by make_unicorn.py — do not edit",
        "// Original pixel-art unicorn for the cyberpunkunicorn OS boot screen.",
        "#pragma once",
        "#include <stdint.h>",
        "",
        f"#define UNI_W {w}",
        f"#define UNI_H {h}",
        f"#define UNI_FRAMES {len(FRAMES)}",
        "#define UNI_TRANSPARENT 0",
        "#define UNI_RAINBOW 1",
        "",
        "// RGB565 colour for each palette index (0 and 1 are special).",
        "static const uint16_t uni_palette[] = {",
    ]
    by_idx = sorted(PALETTE.values(), key=lambda v: v[0])
    out.append("    " + ", ".join("0x%04X" % (rgb565(rgb) if rgb else 0) for _, rgb in by_idx) + ",")
    out.append("};")
    out.append("")
    out.append("static const uint8_t uni_frames[UNI_FRAMES][UNI_H * UNI_W] = {")
    for f in FRAMES:
        out.append("    {")
        for row in f:
            out.append("        " + ", ".join(str(PALETTE[ch][0]) for ch in row) + ",")
        out.append("    },")
    out.append("};")
    open(os.path.join(HERE, "..", "apps", "boot", "unicorn_sprite.h"), "w").write("\n".join(out) + "\n")


def boot_preview(w, h):
    W, H = 320, 240
    img = Image.new("RGB", (W, H), (8, 0, 20))
    d = ImageDraw.Draw(img)
    # sky gradient
    for y in range(34, 150):
        k = (y - 34) / 116
        d.line([(0, y), (W, y)], fill=(int(40 + 60 * k), 0, int(80 - 30 * k)))
    # sun with stripes
    cx, cy, r = 220, 120, 44
    for y in range(cy - r, cy + 1):
        dx = int((r * r - (y - cy) ** 2) ** 0.5)
        stripe = y > cy - 20 and (y - (cy - 20)) % 7 < 2
        if not stripe:
            k = (y - (cy - r)) / r
            d.line([(cx - dx, y), (cx + dx, y)], fill=(255, int(220 - 160 * k), int(60 + 120 * k)))
    # grid floor
    horizon = 150
    d.rectangle([0, horizon, W, 204], fill=(10, 0, 25))
    for i in range(1, 9):
        y = horizon + int((i / 8) ** 2 * 54)
        d.line([(0, y), (W, y)], fill=(0, 200, 255))
    for i in range(-10, 11):
        d.line([(W // 2 + i * 10, horizon), (W // 2 + i * 60, 204)], fill=(255, 0, 200))
    # rainbow trail
    ux, uy = 70, 116
    for band in range(6):
        col = rainbow(0, band * 60)
        for x in range(0, ux + 10):
            y = uy + 40 + band * 3 + int(3 * __import__("math").sin((x + band) / 9))
            d.rectangle([x, y, x, y + 2], fill=col)
    img.paste(render(FRAMES[0], 0), (ux, uy), render(FRAMES[0], 0))
    # title + loading bar
    d.text((24, 8), "CYBERPUNK UNICORN", fill=(255, 0, 200))
    d.rectangle([20, 214, 300, 224], outline=(0, 255, 255))
    for x in range(21, 200):
        d.line([(x, 215), (x, 223)], fill=rainbow(0, x * 2))
    d.text((20, 228), "Reticulating rainbows... 67%", fill=(255, 255, 255))
    img.save(os.path.join(HERE, "boot_preview.png"))


def main():
    w, h = check()
    write_header(w, h)
    sheet = Image.new("RGBA", (w * 4 * 2 + 20, h * 4), (8, 0, 20, 255))
    for i, f in enumerate(FRAMES):
        im = render(f, i * 90, 4)
        sheet.paste(im, (i * (w * 4 + 20), 0), im)
    sheet.save(os.path.join(HERE, "unicorn_preview.png"))
    boot_preview(w, h)
    print(f"unicorn {w}x{h} x{len(FRAMES)} frames -> unicorn_sprite.h ({w * h * len(FRAMES)} bytes)")


if __name__ == "__main__":
    main()
