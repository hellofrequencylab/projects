#!/usr/bin/env python3
"""Stitch pictures and GIFs into one looping 240x135 GIF with transitions between them.

    python3 tools/make_reel.py -o reel.gif a.gif b.jpg c.webp ...
    python3 tools/make_reel.py -o reel.gif --hold 3 --dest /Volumes/CIRCUITPY/slides ~/Pictures/Faves

Each item is shown whole (letterboxed over a blurred copy of itself), animated GIFs play,
and a different wipe carries you into the next one. Wipes only redraw the part of the screen
that changes, which keeps the file small enough for the board.
"""
import argparse
import math
import os
import shutil

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageOps, ImageSequence

W, H = 240, 135
EXTS = {".gif", ".png", ".jpg", ".jpeg", ".webp", ".bmp"}


def compose(img):
    """Whole image centred over a blurred, darkened, screen-filling copy of itself."""
    img = ImageOps.exif_transpose(img).convert("RGB")
    bg = ImageOps.fit(img, (W, H), Image.LANCZOS).filter(ImageFilter.GaussianBlur(10))
    bg = ImageEnhance.Brightness(bg).enhance(0.55)
    fg = img.copy()
    fg.thumbnail((W, H), Image.LANCZOS)
    bg.paste(fg, ((W - fg.width) // 2, (H - fg.height) // 2))
    return bg


def clip_frames(path, hold, fps):
    """[(frame, ms)] for one item: a still held for `hold` s, or the GIF's frames (resampled)."""
    with Image.open(path) as im:
        n = getattr(im, "n_frames", 1)
        if n == 1:
            return [(compose(im), int(hold * 1000))]
        out, t, next_t = [], 0, 0
        step = 1000 / fps
        for frame in ImageSequence.Iterator(im):
            ms = frame.info.get("duration", 100) or 100
            if t >= next_t:
                out.append([compose(frame), 0])
                next_t += step
            if out:
                out[-1][1] += ms
            t += ms
            if t >= max(hold * 1000, 1500) and t >= 1000 * 6:
                break
        return [(f, max(40, int(ms))) for f, ms in out]


# ---- transitions: mask(t) is white where the NEXT image shows, t from 0 to 1 ----

def ease(t):
    return t * t * (3 - 2 * t)


def wipe(direction):
    def mask(t):
        m = Image.new("L", (W, H), 0)
        d = ImageDraw.Draw(m)
        e = ease(t)
        if direction == "left":
            d.rectangle([W - e * W, 0, W, H], fill=255)
        elif direction == "right":
            d.rectangle([0, 0, e * W, H], fill=255)
        elif direction == "down":
            d.rectangle([0, 0, W, e * H], fill=255)
        else:
            d.rectangle([0, H - e * H, W, H], fill=255)
        return m
    return mask


def iris(t):
    m = Image.new("L", (W, H), 0)
    r = ease(t) * math.hypot(W, H) / 2
    ImageDraw.Draw(m).ellipse([W / 2 - r, H / 2 - r, W / 2 + r, H / 2 + r], fill=255)
    return m


def diagonal(t):
    m = Image.new("L", (W, H), 0)
    k = ease(t) * (W + H)
    ImageDraw.Draw(m).polygon([(0, 0), (k, 0), (0, k)], fill=255)
    return m


def blinds(t):
    m = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(m)
    band = 15
    for y in range(0, H, band):
        d.rectangle([0, y, W, y + ease(t) * band], fill=255)
    return m


def heart(t):
    m = Image.new("L", (W, H), 0)
    s = ease(t) * 1.6 * max(W, H) / 32
    pts = []
    for i in range(80):
        a = 2 * math.pi * i / 80
        x = 16 * math.sin(a) ** 3
        y = 13 * math.cos(a) - 5 * math.cos(2 * a) - 2 * math.cos(3 * a) - math.cos(4 * a)
        pts.append((W / 2 + x * s, H / 2 - y * s - 2 * s))
    if s > 0:
        ImageDraw.Draw(m).polygon(pts, fill=255)
    return m


def checker(t):
    m = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(m)
    cell = 30
    for gy, y in enumerate(range(0, H, cell)):
        for gx, x in enumerate(range(0, W, cell)):
            delay = ((gx + gy) % 4) / 4 * 0.5
            p = min(1, max(0, (t - delay) / 0.5))
            if p > 0:
                c = cell * p / 2
                d.rectangle([x + cell / 2 - c, y + cell / 2 - c, x + cell / 2 + c, y + cell / 2 + c], fill=255)
    return m


TRANSITIONS = [heart, wipe("left"), iris, diagonal, blinds, wipe("down"), checker, wipe("right"), iris]


def build(paths, hold, fps, steps, step_ms):
    """[(frame, ms, group)] where each group is one clip plus the transition out of it."""
    clips = [clip_frames(p, hold, fps) for p in paths]
    frames = []
    for i, clip in enumerate(clips):
        frames += [(f, ms, i) for f, ms in clip]
        a = clip[-1][0]
        b = clips[(i + 1) % len(clips)][0][0]
        mask_fn = TRANSITIONS[i % len(TRANSITIONS)]
        for s in range(1, steps + 1):
            m = mask_fn(s / (steps + 1)).filter(ImageFilter.GaussianBlur(1.5))
            frames.append((Image.composite(b, a, m), step_ms, i))
    return frames


def save(frames, out, colors):
    # One palette per group (built from that clip and the next picture), so pixels a wipe
    # hasn't reached yet stay byte-identical and the encoder only stores what changed.
    palettes = {}
    groups = sorted(set(g for _, _, g in frames))
    for g in groups:
        members = [f for f, _, gg in frames if gg == g]
        nxt = next(f for f, _, gg in frames if gg == (g + 1) % len(groups))
        sample = [members[0], members[len(members) // 2], members[-1], nxt]
        sheet = Image.new("RGB", (W * len(sample), H))
        for k, im in enumerate(sample):
            sheet.paste(im, (k * W, 0))
        palettes[g] = sheet.quantize(colors=colors, method=Image.MEDIANCUT)
    imgs = [f.quantize(palette=palettes[g], dither=Image.Dither.NONE) for f, _, g in frames]
    imgs[0].save(out, save_all=True, append_images=imgs[1:], duration=[ms for _, ms, _ in frames],
                 loop=0, optimize=False, disposal=1)
    return os.path.getsize(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="+")
    ap.add_argument("-o", "--out", default="reel.gif")
    ap.add_argument("--hold", type=float, default=2.5, help="seconds per still (default 2.5)")
    ap.add_argument("--fps", type=float, default=10, help="frame rate for animated clips")
    ap.add_argument("--steps", type=int, default=7, help="frames per transition")
    ap.add_argument("--max-kb", type=int, default=450, help="shrink until the file fits this")
    ap.add_argument("--dest", help="also copy the result here, e.g. /Volumes/CIRCUITPY/slides")
    args = ap.parse_args()

    files = []
    for p in args.paths:
        p = os.path.expanduser(p)
        if os.path.isdir(p):
            files += [os.path.join(p, f) for f in sorted(os.listdir(p)) if os.path.splitext(f)[1].lower() in EXTS]
        else:
            files.append(p)

    # Shrink colours, then transition length, until it fits the size budget
    for colors, steps in ((256, args.steps), (192, args.steps), (128, args.steps), (96, args.steps),
                          (64, args.steps), (64, 5), (48, 4), (32, 4)):
        frames = build(files, args.hold, args.fps, steps, 55)
        size = save(frames, args.out, colors)
        print("%d frames, %d colours, %d-step transitions: %d KB" % (len(frames), colors, steps, size // 1024))
        if size <= args.max_kb * 1024:
            break
    total = sum(f[1] for f in frames) / 1000
    print("wrote %s: %.1f s loop, %d KB" % (args.out, total, size // 1024))
    if args.dest:
        dest = os.path.join(args.dest, os.path.basename(args.out))
        shutil.copy(args.out, dest)
        print("copied to", dest)


if __name__ == "__main__":
    main()
