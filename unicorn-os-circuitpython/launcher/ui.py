# Shared helpers for the launcher and its apps: colours, drawing, button, LED.
import time
import board
import digitalio
import displayio
import keypad
import neopixel_write
import terminalio
import vectorio
from adafruit_display_text import label
from adafruit_ticks import ticks_ms, ticks_diff

W, H = 240, 135

# Unicorn palette: pastels on a deep plum night sky
BG = 0x1A0F2E
PANEL = 0x2E1D4F
LINE = 0x4B3672
FG = 0xFFF4FB
DIM = 0xB9A5DA
ACCENT = 0x7FDBFF   # baby blue
GOOD = 0x8EF0C5     # mint
WARN = 0xFFD98A     # butter
BAD = 0xFF8FB1      # rose
PURPLE = 0xC9A7FF   # lavender
PINK = 0xFF9EDB     # bubblegum
GOLD = 0xFFD24D
RAINBOW = (0xFF8FB1, 0xFFC48A, 0xFFF08A, 0x8EF0C5, 0x7FDBFF, 0xC9A7FF)

display = board.DISPLAY

# Survives app switches (apps are unloaded when you leave them)
state = {}
web_poll = None  # set by webui.start(); run from Button.poll so the web panel works in every app

_pals = {}


def pal(color):
    p = _pals.get(color)
    if p is None:
        p = displayio.Palette(1)
        p[0] = color
        _pals[color] = p
    return p


def rect(x, y, w, h, color):
    return vectorio.Rectangle(pixel_shader=pal(color), width=max(1, int(w)),
                              height=max(1, int(h)), x=int(x), y=int(y))


def circle(x, y, r, color):
    return vectorio.Circle(pixel_shader=pal(color), radius=max(1, int(r)), x=int(x), y=int(y))


def poly(points, color, x=0, y=0):
    return vectorio.Polygon(pixel_shader=pal(color), points=points, x=x, y=y)


def lbl(text, x, y, color=FG, scale=1, anchor=(0.0, 0.5), bg=None):
    t = label.Label(terminalio.FONT, text=text, color=color, scale=scale,
                    anchor_point=anchor, anchored_position=(int(x), int(y)))
    if bg is not None:
        t.background_color = bg
        t.padding_left = t.padding_right = 3
        t.padding_top = t.padding_bottom = 1
    return t


def screen():
    """New full-screen group with the background filled, shown immediately."""
    g = displayio.Group()
    g.append(rect(0, 0, W, H, BG))
    display.root_group = g
    return g


def star(x, y, r, color):
    """Four-point sparkle."""
    k = max(1, r // 3)
    return poly([(0, -r), (k, -k), (r, 0), (k, k), (0, r), (-k, k), (-r, 0), (-k, -k)], color, int(x), int(y))


def rainbow_bar(g, x, y, w, h=2):
    seg = w / len(RAINBOW)
    for i, c in enumerate(RAINBOW):
        g.append(rect(x + i * seg, y, seg + 1, h, c))


def header(g, title, color=PINK, right=""):
    """Title bar with a rainbow underline. Returns (title_label, right_label)."""
    rainbow_bar(g, 0, 16, W, 2)
    g.append(star(6, 8, 4, GOLD))
    t = lbl(title, 14, 8, color)
    r = lbl(right, W - 6, 8, DIM, anchor=(1.0, 0.5))
    g.append(t)
    g.append(r)
    return t, r


def sprite(art, colors, x=0, y=0, scale=1):
    """Pixel art from strings: '.' is see-through, other letters index `colors`."""
    keys = sorted(colors)
    bmp = displayio.Bitmap(len(art[0]), len(art), len(keys) + 1)
    p = displayio.Palette(len(keys) + 1)
    p.make_transparent(0)
    for n, k in enumerate(keys):
        p[n + 1] = colors[k]
    for yy, row in enumerate(art):
        for xx, ch in enumerate(row):
            if ch != ".":
                bmp[xx, yy] = keys.index(ch) + 1
    grp = displayio.Group(x=int(x), y=int(y), scale=scale)
    grp.append(displayio.TileGrid(bmp, pixel_shader=p))
    return grp


UNICORN = (
    ".........Y..",
    "........YY..",
    "....PP.YY...",
    "...PLLWWW...",
    "..PLLWWWWW..",
    "..PLWWWKWW..",
    ".PLLWWWWWWW.",
    ".PMLWWWWBWWW",
    ".MMLWWWWWWW.",
    "..MLWWWW....",
    "..MMWWWW....",
    "...MWWWW....",
)
UNICORN_COLORS = {"Y": GOLD, "P": PINK, "L": PURPLE, "M": GOOD, "W": FG, "K": 0x2A1840, "B": BAD}


def clip(text, n):
    return text if len(text) <= n else text[: n - 1] + "~"


def signal_level(rssi):
    if rssi >= -55:
        return 4
    if rssi >= -67:
        return 3
    if rssi >= -75:
        return 2
    if rssi >= -85:
        return 1
    return 0


LEVEL_COLORS = (BAD, BAD, WARN, ACCENT, GOOD)  # signal bars, weakest to strongest


class Bars:
    """Four-step signal-strength icon."""

    def __init__(self, g, x, y):
        self.bars = []
        for i in range(4):
            h = 3 + i * 3
            b = rect(x + i * 4, y + 12 - h, 3, h, LINE)
            g.append(b)
            self.bars.append(b)

    def set(self, rssi):
        lvl = signal_level(rssi) if rssi is not None else 0
        on = pal(LEVEL_COLORS[lvl])
        off = pal(LINE)
        for i, b in enumerate(self.bars):
            b.pixel_shader = on if i < lvl else off


class Button:
    """The BOOT button. poll() returns 'short', 'double', 'long' or None.

    Double-tap means "back" everywhere. A single tap is reported once the double-tap
    window has passed. Long presses fire while the button is still held.
    keypad queues presses in the background, so taps during slow work still count.
    """

    LONG_MS = 700
    DOUBLE_MS = 350

    def __init__(self):
        self._keys = keypad.Keys((board.BUTTON,), value_when_pressed=False, pull=True)
        self._down = None
        self._long_fired = False
        self._tap = None  # first tap waiting to see if a second one follows

    def poll(self):
        if web_poll is not None:
            web_poll()
        if state.get("launch") and state.get("app", "menu") != "menu":
            return "double"  # the web panel asked for another app: step aside
        ev = self._keys.events.get()
        if ev:
            if ev.pressed:
                self._down = ev.timestamp
                self._long_fired = False
            elif ev.released and self._down is not None:
                held = ticks_diff(ev.timestamp, self._down)
                self._down = None
                if not self._long_fired:
                    if held >= self.LONG_MS:
                        self._tap = None
                        return "long"
                    if self._tap is not None and ticks_diff(ev.timestamp, self._tap) < self.DOUBLE_MS:
                        self._tap = None
                        return "double"
                    self._tap = ev.timestamp
        now = ticks_ms()
        if (self._down is not None and not self._long_fired
                and ticks_diff(now, self._down) >= self.LONG_MS):
            self._long_fired = True
            self._tap = None
            return "long"
        if self._tap is not None and self._down is None and ticks_diff(now, self._tap) >= self.DOUBLE_MS:
            self._tap = None
            return "short"
        return None

    def wait(self, seconds):
        """Sleep up to `seconds` while watching the button; return the first event."""
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            e = self.poll()
            if e:
                return e
            time.sleep(0.01)
        return None

    def clear(self):
        self._keys.events.clear()
        self._down = None
        self._long_fired = True
        self._tap = None


class Pixel:
    """The onboard RGB LED, driven without the neopixel library."""

    def __init__(self, brightness=0.12):
        self.brightness = brightness
        try:
            self._pwr = digitalio.DigitalInOut(board.NEOPIXEL_POWER)
            self._pwr.switch_to_output(True)
        except ValueError:
            self._pwr = None  # already powered by the board
        self._pin = digitalio.DigitalInOut(board.NEOPIXEL)
        self._pin.switch_to_output()

    def color(self, rgb):
        k = self.brightness
        r, g, b = (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255
        neopixel_write.neopixel_write(self._pin, bytearray((int(g * k), int(r * k), int(b * k))))

    def off(self):
        self.color(0)


def blend(c1, c2, t):
    t = 0 if t < 0 else 1 if t > 1 else t
    out = 0
    for shift in (16, 8, 0):
        a = (c1 >> shift) & 255
        b = (c2 >> shift) & 255
        out |= int(a + (b - a) * t) << shift
    return out


# ---------- Settings, kept in the chip's non-volatile memory (survive power-off) ----------
# The CIRCUITPY drive is read-only to code while it's plugged into a computer, so settings
# live in microcontroller.nvm instead. Layout: [0] menu position, [64:576] settings,
# [1024:] altimeter log.

import json
import microcontroller

SETTINGS_AT = 64
SETTINGS_MAX = 510
DEFAULTS = {
    "screen": 1.0,          # display backlight 0.25-1.0
    "led_level": 2,         # LED panel brightness step (index into Light Show's levels)
    "status_led": 1,        # onboard RGB LED: 0 off, 1 low, 2 medium, 3 high
    "wifi": True,
    "units": "imperial",    # or "metric"
    "slide_seconds": 1.0,   # how long still pictures stay up
    "log_every": 30,        # altimeter log interval, seconds
}
STATUS_LED_LEVELS = (0.0, 0.06, 0.15, 0.35)


def load_settings():
    s = dict(DEFAULTS)
    try:
        n = microcontroller.nvm[SETTINGS_AT] | (microcontroller.nvm[SETTINGS_AT + 1] << 8)
        if 0 < n <= SETTINGS_MAX:
            s.update(json.loads(bytes(microcontroller.nvm[SETTINGS_AT + 2:SETTINGS_AT + 2 + n])))
    except (ValueError, KeyError):
        pass  # blank or corrupt: fall back to defaults
    return s


def save_settings():
    raw = json.dumps(settings).encode()
    if len(raw) > SETTINGS_MAX:
        raise ValueError("settings too large")
    microcontroller.nvm[SETTINGS_AT:SETTINGS_AT + 2 + len(raw)] = bytes((len(raw) & 255, len(raw) >> 8)) + raw


# Wi-Fi network chosen in the Wi-Fi Setup app lives at [600:1000] in nvm
WIFI_AT = 600
WIFI_MAX = 398


def load_wifi():
    """(ssid, password) from the chip, else from settings.toml, else (None, None)."""
    try:
        n = microcontroller.nvm[WIFI_AT] | (microcontroller.nvm[WIFI_AT + 1] << 8)
        if 0 < n <= WIFI_MAX:
            d = json.loads(bytes(microcontroller.nvm[WIFI_AT + 2:WIFI_AT + 2 + n]))
            return d["ssid"], d["pw"]
    except (ValueError, KeyError):
        pass
    import os
    return os.getenv("CIRCUITPY_WIFI_SSID"), os.getenv("CIRCUITPY_WIFI_PASSWORD")


def save_wifi(ssid, pw):
    raw = json.dumps({"ssid": ssid, "pw": pw}).encode()
    if len(raw) > WIFI_MAX:
        raise ValueError("network name or password too long")
    microcontroller.nvm[WIFI_AT:WIFI_AT + 2 + len(raw)] = bytes((len(raw) & 255, len(raw) >> 8)) + raw


def wifi_networks():
    """Every saved network, in order: the one from Wi-Fi Setup, then settings.toml's
    CIRCUITPY_WIFI_*, then WIFI2_* and WIFI3_* (extra places like a cafe)."""
    import os
    nets = []
    try:
        n = microcontroller.nvm[WIFI_AT] | (microcontroller.nvm[WIFI_AT + 1] << 8)
        if 0 < n <= WIFI_MAX:
            d = json.loads(bytes(microcontroller.nvm[WIFI_AT + 2:WIFI_AT + 2 + n]))
            nets.append((d["ssid"], d["pw"]))
    except (ValueError, KeyError):
        pass
    for prefix in ("CIRCUITPY_WIFI", "WIFI2", "WIFI3"):
        ssid = os.getenv(prefix + "_SSID")
        if ssid and ssid not in [s for s, _ in nets]:
            nets.append((ssid, os.getenv(prefix + "_PASSWORD") or ""))
    return nets


def connect_wifi(timeout=20):
    """Join whichever saved network is in range, if Wi-Fi is on. True when connected."""
    import wifi
    if not settings["wifi"]:
        return False
    if wifi.radio.connected:
        return True
    nets = wifi_networks()
    if not nets:
        return False
    try:  # only try networks that are actually around, so each miss doesn't cost a timeout
        here = set(n.ssid for n in wifi.radio.start_scanning_networks())
        wifi.radio.stop_scanning_networks()
        nets = [n for n in nets if n[0] in here] or nets
    except (RuntimeError, OSError):
        pass
    for ssid, pw in nets:
        try:
            wifi.radio.connect(ssid, pw, timeout=timeout)
        except (ConnectionError, OSError, RuntimeError, ValueError):
            continue
        if wifi.radio.connected:
            return True
    return False


def apply_settings(px=None):
    """Push the hardware-facing settings to the hardware."""
    display.brightness = settings["screen"]
    if px is not None:
        px.brightness = STATUS_LED_LEVELS[settings["status_led"]]
    try:
        import wifi
        wifi.radio.enabled = settings["wifi"]
    except (ImportError, RuntimeError):
        pass


def imperial():
    return settings["units"] != "metric"


def temp_text(c):
    return "%d%s" % (round(c * 9 / 5 + 32), "F") if imperial() else "%d%s" % (round(c), "C")


settings = load_settings()
