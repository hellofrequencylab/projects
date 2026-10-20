#!/usr/bin/env python3
"""Shrink photos and GIFs to the Feather's 240x135 screen and copy them to CIRCUITPY/slides.

    python3 tools/add_slides.py ~/Pictures/cat.jpg ~/Downloads/party.gif
    python3 tools/add_slides.py ~/Pictures/Trip            # a whole folder
    python3 tools/add_slides.py --fit photo.png            # letterbox instead of crop

Stills become 8-bit BMPs (~33 KB each); animations stay GIFs (frames are capped to keep
them small). iPhone .heic photos work too (converted with macOS `sips` first).
Needs Pillow:  pip3 install pillow
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageOps, ImageSequence

W, H = 240, 135
DEST = "/Volumes/CIRCUITPY/slides"
MAX_FRAMES = 40
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp", ".heic", ".tif", ".tiff"}


def fit_frame(img, fit):
    img = ImageOps.exif_transpose(img).convert("RGB")
    if fit:
        img.thumbnail((W, H), Image.LANCZOS)
        canvas = Image.new("RGB", (W, H))
        canvas.paste(img, ((W - img.width) // 2, (H - img.height) // 2))
        return canvas
    # Crop toward the upper part of tall pictures, where faces usually are
    return ImageOps.fit(img, (W, H), Image.LANCZOS, centering=(0.5, 0.3))


def convert(src, out_dir, fit):
    base = os.path.splitext(os.path.basename(src))[0]
    safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in base)[:40] or "slide"
    tmp_png = None
    if src.lower().endswith(".heic"):
        tmp_png = os.path.join(tempfile.mkdtemp(), safe + ".png")
        subprocess.run(["sips", "-s", "format", "png", src, "--out", tmp_png],
                       check=True, capture_output=True)
        src = tmp_png
    with Image.open(src) as im:
        frames = getattr(im, "n_frames", 1)
        if frames > 1:
            step = max(1, -(-frames // MAX_FRAMES))
            out, durations = [], []
            for n, frame in enumerate(ImageSequence.Iterator(im)):
                if n % step:
                    continue
                out.append(fit_frame(frame.copy(), fit).quantize(colors=64, method=Image.MEDIANCUT))
                durations.append(max(40, frame.info.get("duration", 100) * step))
            dest = os.path.join(out_dir, safe + ".gif")
            out[0].save(dest, save_all=True, append_images=out[1:], duration=durations,
                        loop=0, optimize=True, disposal=1)
        else:
            dest = os.path.join(out_dir, safe + ".bmp")
            fit_frame(im, fit).quantize(colors=256, method=Image.MEDIANCUT).save(dest)
    if tmp_png:
        shutil.rmtree(os.path.dirname(tmp_png), ignore_errors=True)
    return dest


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="+", help="image files or folders")
    ap.add_argument("--fit", action="store_true", help="letterbox (default: crop to fill the screen)")
    ap.add_argument("--dest", default=DEST, help="where to put them (default: %(default)s)")
    args = ap.parse_args()

    files = []
    for p in args.paths:
        p = os.path.expanduser(p)
        if os.path.isdir(p):
            files += [os.path.join(p, f) for f in sorted(os.listdir(p))
                      if os.path.splitext(f)[1].lower() in IMAGE_EXTS]
        else:
            files.append(p)
    args.dest = os.path.abspath(os.path.expanduser(args.dest))
    if not os.path.isdir(os.path.dirname(args.dest)):
        sys.exit("Board drive not found at %s. Is CIRCUITPY plugged in?" % os.path.dirname(args.dest))
    os.makedirs(args.dest, exist_ok=True)

    with tempfile.TemporaryDirectory() as work:
        for f in files:
            try:
                tmp = convert(f, work, args.fit)
            except Exception as e:  # keep going through the rest of the batch
                print("skip %s: %s" % (f, e))
                continue
            size = os.path.getsize(tmp)
            free = shutil.disk_usage(args.dest).free
            if size > free - 16 * 1024:
                print("STOP: board is full (%d KB free). Delete some slides first." % (free // 1024))
                break
            shutil.copy(tmp, os.path.join(args.dest, os.path.basename(tmp)))
            print("added %-30s %4d KB" % (os.path.basename(tmp), size // 1024))
    print("Board space left: %d KB" % (shutil.disk_usage(args.dest).free // 1024))


if __name__ == "__main__":
    main()
