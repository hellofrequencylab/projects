# Slideshow of pictures and animated GIFs from /slides.
# Tap BOOT for the next slide, hold to show it on the 8x8 LED panel too, double-tap to go back.
# Best results: 240x135 images. tools/add_slides.py on the Mac converts anything for you.
import gc
import os
import struct
import time
import displayio
import gifio
import jpegio
import neopixel_write
import ui

FOLDER = "/slides"
# Each GIF plays through once, then the next slide comes straight up, so slides last as
# long as their animation. Pictures stay up for System > Settings > Slide time. Loops forever.
STILL_SECONDS = ui.settings["slide_seconds"]  # set in System > Settings
GIF_MAX_SECONDS = 15  # safety cap for very long GIFs
CAPTION_SECONDS = min(1.8, STILL_SECONDS * 0.4)

RGB565 = displayio.ColorConverter(input_colorspace=displayio.Colorspace.RGB565_SWAPPED)


def slide_files():
    try:
        names = os.listdir(FOLDER)
    except OSError:
        return []
    exts = (".bmp", ".gif", ".jpg", ".jpeg")
    return sorted(n for n in names if n.lower().endswith(exts) and not n.startswith("."))


def centered(tile, w, h):
    tile.x = (ui.W - w) // 2
    tile.y = (ui.H - h) // 2
    return tile


def empty_screen(btn):
    g = ui.screen()
    ui.header(g, "SLIDESHOW", ui.PINK, "2x tap: back")
    g.append(ui.lbl("No pictures yet", ui.W // 2, 50, ui.FG, scale=2, anchor=(0.5, 0.5)))
    g.append(ui.lbl("Copy .bmp .gif .jpg files into", ui.W // 2, 80, ui.DIM, anchor=(0.5, 0.5)))
    g.append(ui.lbl("CIRCUITPY/slides", ui.W // 2, 96, ui.PINK, anchor=(0.5, 0.5)))
    while btn.wait(1) not in ("long", "double"):
        pass


# ---------- LED panel mirror: the centre square of each slide, shrunk to 8x8 ----------

LED_ZOOM = 0.65   # share of the picture's height the panel shows (smaller = tighter)
LED_FOCUS_Y = 0.4  # where that square sits vertically: faces are usually above centre
LED_SATURATION = 1.2


def crop_square(w, h):
    side = min(w, h) * LED_ZOOM
    x0 = (w - side) / 2
    y0 = min(h - side, max(0, h * LED_FOCUS_Y - side / 2))
    return x0, y0, side / 8


def enhance(colors):
    """Stretch each frame to the panel's full range and push colours apart, so 64 LEDs
    show shapes, not mush."""
    lumas = [(r * 3 + g * 6 + b) // 10 for r, g, b in colors]
    lo, hi = min(lumas), max(lumas)
    k = 255 / (hi - lo) if hi - lo > 30 else 1.0
    if k == 1.0:
        lo = 0
    out = []
    for (r, g, b), y in zip(colors, lumas):
        y2 = (y - lo) * k
        rr = y2 + ((r - lo) * k - y2) * LED_SATURATION
        gg = y2 + ((g - lo) * k - y2) * LED_SATURATION
        bb = y2 + ((b - lo) * k - y2) * LED_SATURATION
        out.append((0 if rr < 0 else 255 if rr > 255 else int(rr),
                    0 if gg < 0 else 255 if gg > 255 else int(gg),
                    0 if bb < 0 else 255 if bb > 255 else int(bb)))
    return out

def rgb565s(v):
    v = ((v & 0xFF) << 8) | (v >> 8)  # byte-swapped RGB565, as gifio and jpegio produce
    return ((v >> 8) & 0xF8, (v >> 3) & 0xFC, (v << 3) & 0xF8)


def square_from_bitmap(bmp, w, h):
    """64 colours from the centre square, each the average of 4 samples."""
    x0, y0, cell = crop_square(w, h)
    out = []
    for cy in range(8):
        for cx in range(8):
            r = g = b = 0
            for sy in (0.25, 0.75):
                for sx in (0.25, 0.75):
                    c = rgb565s(bmp[int(x0 + (cx + sx) * cell), int(y0 + (cy + sy) * cell)])
                    r += c[0]
                    g += c[1]
                    b += c[2]
            out.append((r >> 2, g >> 2, b >> 2))
    return enhance(out)


def square_from_bmp_file(path):
    """Same, read straight from an 8-bit BMP file (OnDiskBitmap can't be read back)."""
    with open(path, "rb") as f:
        head = f.read(54)
        offset, = struct.unpack_from("<I", head, 10)
        hsize, w, h = struct.unpack_from("<Iii", head, 14)
        bpp, = struct.unpack_from("<H", head, 28)
        if bpp != 8:
            return None
        f.seek(14 + hsize)
        palette = f.read(offset - 14 - hsize)
        stride = (w + 3) & ~3
        bottom_up = h > 0
        h = abs(h)
        x0, y0, cell = crop_square(w, h)
        out = []
        for cy in range(8):
            for cx in range(8):
                r = g = b = 0
                for sy in (0.25, 0.75):
                    for sx in (0.25, 0.75):
                        x, y = int(x0 + (cx + sx) * cell), int(y0 + (cy + sy) * cell)
                        f.seek(offset + (h - 1 - y if bottom_up else y) * stride + x)
                        k = f.read(1)[0] * 4
                        b += palette[k]
                        g += palette[k + 1]
                        r += palette[k + 2]
                out.append((r >> 2, g >> 2, b >> 2))
        return enhance(out)


class Mirror:
    """Sends slides to the LED panel, using Light Show's pin, colour curve and brightness."""

    def __init__(self):
        import apps.lightshow as ls
        self.ls = ls
        self.on = ui.state.get("slide_led", False)
        self.pin = None
        self.gam = ls.gamma_table(2.2)
        self.out = bytearray(8 * 8 * 3)

    def toggle(self):
        self.on = not self.on
        ui.state["slide_led"] = self.on
        if not self.on:
            self.off()
        return self.on

    def show(self, colors):
        if not self.on or colors is None:
            return
        if self.pin is None:
            self.pin = self.ls.data_pin()
        k = self.ls.LEVELS[min(len(self.ls.LEVELS) - 1, ui.settings["led_level"])]
        gam, out, order = self.gam, self.out, self.ls.ORDER
        for i, (r, g, b) in enumerate(colors):
            j = order[i] * 3
            out[j] = int(gam[g] * k)
            out[j + 1] = int(gam[r] * k)
            out[j + 2] = int(gam[b] * k)
        neopixel_write.neopixel_write(self.pin, out)

    def off(self):
        if self.pin is not None or "ls_pin" in ui.state:
            self.ls.blank(self.ls.data_pin())


def show_still(path, g):
    """Shows a picture; returns its 8x8 colours for the LED panel (or None)."""
    if path.lower().endswith(".bmp"):
        odb = displayio.OnDiskBitmap(path)
        g.append(centered(displayio.TileGrid(odb, pixel_shader=odb.pixel_shader),
                          odb.width, odb.height))
        return square_from_bmp_file(path)
    decoder = jpegio.JpegDecoder()
    w, h = decoder.open(path)
    scale = 0
    while scale < 3 and ((w >> scale) > ui.W * 2 or (h >> scale) > ui.H * 2):
        scale += 1
    sw, sh = w >> scale, h >> scale
    if sw * sh > ui.W * ui.H * 4:
        raise ValueError("too big: %dx%d" % (w, h))
    bmp = displayio.Bitmap(sw, sh, 65535)
    decoder.decode(bmp, scale=scale)
    g.append(centered(displayio.TileGrid(bmp, pixel_shader=RGB565), sw, sh))
    return square_from_bitmap(bmp, sw, sh)


def run(btn, px):
    files = slide_files()
    if not files:
        empty_screen(btn)
        return

    root = displayio.Group()
    ui.display.root_group = root
    root.append(ui.rect(0, 0, ui.W, ui.H, 0x000000))
    holder = displayio.Group()
    root.append(holder)
    caption = ui.lbl("", ui.W // 2, ui.H - 9, ui.FG, anchor=(0.5, 0.5), bg=0x000000)
    root.append(caption)
    mirror = Mirror()
    try:
        slides(files, holder, caption, btn, px, mirror)
    finally:
        mirror.off()


def toggle_led(mirror, caption):
    caption.text = "LED panel: " + ("on" if mirror.toggle() else "off")
    caption.hidden = False
    return time.monotonic()  # restart the caption timer so the message stays up


def slides(files, holder, caption, btn, px, mirror):
    i = ui.state.get("slide_index", 0) % len(files)
    while True:
        name = files[i]
        path = FOLDER + "/" + name
        while len(holder):
            holder.pop()
        gc.collect()
        caption.text = "%d/%d  %s" % (i + 1, len(files), ui.clip(name, 28))
        caption.hidden = False
        shown_at = time.monotonic()
        px.color(ui.PINK)
        event = None
        try:
            if name.lower().endswith(".gif"):
                event = play_gif(path, holder, caption, shown_at, btn, mirror)
            else:
                colors = show_still(path, holder)
                mirror.show(colors)
                end = shown_at + STILL_SECONDS
                cap_at = shown_at
                while event is None and time.monotonic() < end:
                    if time.monotonic() - cap_at > CAPTION_SECONDS:
                        caption.hidden = True
                    event = btn.wait(0.1)
                    if event == "long":
                        cap_at = toggle_led(mirror, caption)
                        mirror.show(colors)
                        event = None
        except (OSError, ValueError, MemoryError, RuntimeError) as err:
            why = str(err)
            if "width" in why or "too big" in why or isinstance(err, MemoryError):
                why = "too big, use add_slides.py"
            caption.text = "skip %s: %s" % (ui.clip(name, 12), ui.clip(why, 26))
            caption.hidden = False
            event = btn.wait(3)

        if event in ("long", "double"):
            ui.state["slide_index"] = i
            return
        i = (i + 1) % len(files)


def play_gif(path, holder, caption, shown_at, btn, mirror):
    gif = gifio.OnDiskGif(path)
    try:
        # Half-size GIFs (from fit_gifs.py --half) are drawn at 2x to fill the screen
        scale = 2 if gif.width * 2 <= ui.W else 1
        frame = displayio.Group(scale=scale)
        frame.append(displayio.TileGrid(gif.bitmap, pixel_shader=RGB565))
        holder.append(centered(frame, gif.width * scale, gif.height * scale))
        cap_at = shown_at
        for _ in range(gif.frame_count):  # one pass through the animation
            started = time.monotonic()
            delay = gif.next_frame()
            if mirror.on:
                mirror.show(square_from_bitmap(gif.bitmap, gif.width, gif.height))
            now = time.monotonic()
            delay -= now - started  # decoding (and LED) time counts toward the frame's delay
            if now - cap_at > CAPTION_SECONDS:
                caption.hidden = True
            if now - shown_at > GIF_MAX_SECONDS:
                return None
            event = btn.wait(max(delay, 0.02))
            if event == "long":
                cap_at = toggle_led(mirror, caption)
            elif event:
                return event
        return None
    finally:
        holder.pop()
        gif.deinit()
