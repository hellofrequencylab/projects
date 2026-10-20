# Light Show: drives an 8x8 WS2812B panel with the scenes in scenes.py, and mirrors the
# panel on screen. Tap BOOT for the next scene, hold for brightness, double-tap to go back.
#
# Wiring: panel V+ -> USB, IN -> D5, V- -> GND (bottom-left pads on the panel).
# settings.toml (all optional):
#   LS_PIN = "D5"            # data pin
#   LS_ZIGZAG = "0"          # "1" if rows run back and forth instead of all left-to-right
#   LS_ROTATE = "0"          # 0, 90, 180 or 270
import os
import time
import board
import digitalio
import displayio
import neopixel_write
import scenes
import ui

N = scenes.N
PIN = os.getenv("LS_PIN") or "D5"
ZIGZAG = (os.getenv("LS_ZIGZAG") or "0") == "1"
ROTATE = int(os.getenv("LS_ROTATE") or 0) % 360
# Brightness steps for hold. Kept low for USB power: 64 LEDs at full white draw ~3.8 A
LEVELS = (0.04, 0.08, 0.15, 0.25, 0.4)

_gamma_tables = {}


def gamma_table(g):
    """LEDs look far too bright at low values. A higher curve gives deeper darks and
    punchier colour; a lower one gives a softer, glowier wash. Scenes pick their own."""
    t = _gamma_tables.get(g)
    if t is None:
        t = _gamma_tables[g] = bytes(int((i / 255) ** g * 255 + 0.5) for i in range(256))
    return t


def panel_index(x, y):
    """Map screen-style (x, y) to the LED's position on the panel's data chain."""
    for _ in range(ROTATE // 90):
        x, y = N - 1 - y, x
    if ZIGZAG and y % 2:
        x = N - 1 - x
    return y * N + x


ORDER = [panel_index(i % N, i // N) for i in range(N * N)]


def rgb565(c):
    return ((c[0] & 0xF8) << 8) | ((c[1] & 0xFC) << 3) | (c[2] >> 3)


def data_pin():
    # Kept for the whole session and left driven low between visits: releasing it let
    # the line float, and the panel could hold its last frame.
    pin = ui.state.get("ls_pin")
    if pin is None:
        pin = digitalio.DigitalInOut(getattr(board, PIN))
        pin.switch_to_output(False)
        ui.state["ls_pin"] = pin
    return pin


def blank(pin):
    off = bytearray(N * N * 3)
    for _ in range(2):  # twice, so a frame cut short can't leave LEDs lit
        neopixel_write.neopixel_write(pin, off)
        time.sleep(0.002)
    pin.value = False


def run(btn, px):
    pin = data_pin()
    out = bytearray(N * N * 3)

    g = ui.screen()
    ui.header(g, "LIGHT SHOW", ui.PINK, "pin " + PIN)
    # On-screen mirror of the panel
    g.append(ui.rect(8, 23, 8 * 13 + 2, 8 * 13 + 2, ui.LINE))
    preview = displayio.Bitmap(N, N, 65536)
    tile = displayio.TileGrid(preview, pixel_shader=displayio.ColorConverter(
        input_colorspace=displayio.Colorspace.RGB565))
    big = displayio.Group(scale=13, x=9, y=24)
    big.append(tile)
    g.append(big)
    name = ui.lbl("", 124, 36, ui.FG, scale=2)
    g.append(name)
    step = ui.lbl("", 124, 54, ui.DIM)
    g.append(step)
    g.append(ui.lbl("tap: next", 124, 72, ui.DIM))
    g.append(ui.lbl("2x tap: back", 124, 86, ui.DIM))
    g.append(ui.lbl("hold: bright", 124, 100, ui.DIM))
    level = min(len(LEVELS) - 1, ui.settings["led_level"])
    bars = []
    for n in range(len(LEVELS)):
        b = ui.rect(124 + n * 14, 126 - (n + 1) * 2, 11, (n + 1) * 2, ui.LINE)
        g.append(b)
        bars.append(b)

    def show_level():
        for n, b in enumerate(bars):
            b.pixel_shader = ui.pal(ui.WARN if n <= level else ui.LINE)

    show_level()

    all_scenes = ui.state.setdefault("ls_scenes", [cls() for cls in scenes.ALL])
    index = ui.state.get("ls_index", 0) % len(all_scenes)
    buf = [(0, 0, 0)] * (N * N)
    t = ui.state.get("ls_clock", 0.0)  # scene clock; runs faster or slower with the web speed slider
    last_tick = time.monotonic()
    last_preview = 0

    def select(i):
        name.text = ui.clip(all_scenes[i].name, 9)
        step.text = "%d of %d" % (i + 1, len(all_scenes))
        return gamma_table(getattr(all_scenes[i], "gamma", 2.2))

    gam = select(index)

    try:
        while True:
            now = time.monotonic()
            t += (now - last_tick) * ui.state.get("ls_speed", 1.0)
            last_tick = now
            ui.state["ls_clock"] = t
            # follow scene and brightness changes made on the web panel
            if ui.state.get("ls_index", index) != index:
                index = ui.state["ls_index"] % len(all_scenes)
                gam = select(index)
            if ui.settings["led_level"] != level:
                level = min(len(LEVELS) - 1, ui.settings["led_level"])
                show_level()
            scene = all_scenes[index]
            scene.frame(t, buf)
            k = LEVELS[level]
            for i, (r, gg, b) in enumerate(buf):
                j = ORDER[i] * 3
                out[j] = int(gam[gg] * k)      # WS2812B wants green, red, blue
                out[j + 1] = int(gam[r] * k)
                out[j + 2] = int(gam[b] * k)
            neopixel_write.neopixel_write(pin, out)
            if now - last_preview > 0.08:  # the screen doesn't need every frame
                last_preview = now
                for i, c in enumerate(buf):
                    preview[i % N, i // N] = rgb565(c)
                sub = getattr(scene, "label", None)  # e.g. which chakra
                if sub and step.text != sub:
                    step.text = sub
                px.color(buf[27][0] << 16 | buf[27][1] << 8 | buf[27][2])
            e = btn.wait(0.02)
            if e == "double":
                return
            if e == "long":
                level = (level + 1) % len(LEVELS)
                ui.settings["led_level"] = level
                ui.save_settings()
                show_level()
            elif e == "short":
                index = (index + 1) % len(all_scenes)
                ui.state["ls_index"] = index
                gam = select(index)
    finally:
        blank(pin)
