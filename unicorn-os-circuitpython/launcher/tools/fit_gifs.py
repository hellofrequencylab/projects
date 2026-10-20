#!/usr/bin/env python3
"""Crop animated GIFs to the Feather's 240x135 screen and squeeze each under a size budget.

    python3 tools/fit_gifs.py --budget 90 --out slides ~/Downloads/a.gif ~/Downloads/b.gif

For each GIF it tries progressively leaner settings (frame rate, length, colours, lossy
compression) until the result fits. Single-frame files become BMP stills.
Needs Pillow and gifsicle:  pip3 install pillow   brew install gifsicle
"""
import argparse
import os
import subprocess
import tempfile

from PIL import Image, ImageOps, ImageSequence

W, H = 240, 135

# (frames per second, max seconds, gifsicle --lossy, colours): best quality first
LADDER = [
    (15, 6, 30, 128), (12, 5, 50, 128), (10, 5, 70, 96), (10, 4, 90, 64),
    (8, 4, 110, 64), (8, 3, 140, 64), (6, 3, 170, 48), (5, 2.5, 200, 32), (4, 2, 200, 32),
]


def crop(frame, size=(W, H)):
    # Crop toward the upper part of tall pictures, where faces usually are
    return ImageOps.fit(frame.convert("RGB"), size, Image.LANCZOS, centering=(0.5, 0.3))


def load(path, size=(W, H)):
    """[(rgb_frame, ms)] cropped to `size`, at the source's own timing."""
    out = []
    with Image.open(path) as im:
        for frame in ImageSequence.Iterator(im):
            ms = frame.info.get("duration", 100)
            if not ms or ms < 20:  # browsers treat 0-10 ms as 100 ms
                ms = 100
            out.append((crop(frame, size), ms))
    return out


def resample(frames, fps, max_s):
    """Pick frames at `fps`, stopping at `max_s` seconds; durations stretch to match."""
    step = 1000 / fps
    out, t, next_t = [], 0, 0
    for img, ms in frames:
        if t >= max_s * 1000:
            break
        if t >= next_t:
            out.append([img, 0])
            next_t += step
        out[-1][1] += ms
        t += ms
    return [(img, max(20, int(ms))) for img, ms in out]


def encode(frames, colors, lossy, dest):
    # One shared palette across the clip so still areas stay identical between frames,
    # then gifsicle stores only the pixels that change.
    picks = frames[:: max(1, len(frames) // 6)][:6]
    fw, fh = frames[0][0].size
    sheet = Image.new("RGB", (fw * len(picks), fh))
    for i, (img, _) in enumerate(picks):
        sheet.paste(img, (i * fw, 0))
    pal = sheet.quantize(colors=colors, method=Image.MEDIANCUT)
    imgs = [img.quantize(palette=pal, dither=Image.Dither.NONE) for img, _ in frames]
    with tempfile.NamedTemporaryFile(suffix=".gif", delete=False) as tmp:
        raw = tmp.name
    imgs[0].save(raw, save_all=True, append_images=imgs[1:], duration=[ms for _, ms in frames],
                 loop=0, disposal=1)
    subprocess.run(["gifsicle", "-O3", "--lossy=%d" % lossy, "--colors", str(colors),
                    "--no-comments", "--no-names", "--no-warnings", "-o", dest, raw], check=True)
    os.unlink(raw)
    return os.path.getsize(dest)


def fit(path, out_dir, budget_kb, half=False):
    base = os.path.splitext(os.path.basename(path))[0]
    safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in base).strip("_")[:40] or "slide"
    frames = load(path)
    if len(frames) == 1:
        dest = os.path.join(out_dir, safe + ".bmp")
        frames[0][0].quantize(colors=256, method=Image.MEDIANCUT).save(dest)
        return dest, "still", os.path.getsize(dest)
    if half:
        # Stored at 120x68; the slideshow draws GIFs this small at double size
        frames = load(path, (W // 2, (H + 1) // 2))
    dest = os.path.join(out_dir, safe + ".gif")
    for fps, max_s, lossy, colors in LADDER:
        clip = resample(frames, fps, max_s)
        size = encode(clip, colors, lossy, dest)
        desc = "%d frames @%dfps, %.1fs, %d colours, lossy %d" % (
            len(clip), fps, sum(ms for _, ms in clip) / 1000, colors, lossy)
        if size <= budget_kb * 1024:
            break
    return dest, desc, size


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--out", required=True, help="output folder")
    ap.add_argument("--budget", type=int, default=90, help="KB per animated GIF (default 90)")
    ap.add_argument("--half", action="store_true",
                    help="store animations at 120x68 (shown at 2x): smoother and more colourful, chunkier pixels")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    total = 0
    for p in args.paths:
        dest, desc, size = fit(os.path.expanduser(p), args.out, args.budget, args.half)
        total += size
        print("%-34s %4d KB  %s" % (os.path.basename(dest), size // 1024, desc))
    print("total %d KB" % (total // 1024))


if __name__ == "__main__":
    main()
