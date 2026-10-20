# Light-show scenes for the 8x8 LED panel.
# Each scene has a `params` dict (colours, speed, text...) and draws into a list of 64
# (r, g, b) tuples, row by row from the top-left. Everything a future web editor needs to
# change lives in `params`; nothing here touches hardware, so it can run anywhere.
import math
import random

N = 8
SIN = [int(127.5 + 127.5 * math.sin(i * math.pi / 128)) for i in range(256)]


def hyp(a, b):
    return math.sqrt(a * a + b * b)  # CircuitPython's math has no hypot()


def wheel(pos):
    """0-255 around the colour wheel."""
    pos &= 255
    if pos < 85:
        return (255 - pos * 3, pos * 3, 0)
    if pos < 170:
        pos -= 85
        return (0, 255 - pos * 3, pos * 3)
    pos -= 170
    return (pos * 3, 0, 255 - pos * 3)


def scale(c, k):
    return (int(c[0] * k), int(c[1] * k), int(c[2] * k))


class Scene:
    name = "scene"
    params = {}
    gamma = 2.2

    def __init__(self):
        self.params = dict(self.params)  # each instance gets its own copy

    def frame(self, t, buf):
        pass


class Rainbow(Scene):
    name = "Rainbow"
    params = {"speed": 1.0, "spread": 12}

    def frame(self, t, buf):
        base = int(t * 60 * self.params["speed"])
        s = self.params["spread"]
        for y in range(N):
            for x in range(N):
                buf[y * N + x] = wheel(base + (x + y) * s)


class Plasma(Scene):
    name = "Plasma"
    params = {"speed": 1.0}

    def frame(self, t, buf):
        k = int(t * 40 * self.params["speed"])
        for y in range(N):
            for x in range(N):
                v = SIN[(x * 32 + k) & 255] + SIN[(y * 28 - k) & 255] + SIN[((x + y) * 20 + k * 2) & 255]
                buf[y * N + x] = wheel(v // 3 + k)


class Fire(Scene):
    name = "Fire"
    params = {"speed": 1.0, "cooling": 55}

    def __init__(self):
        super().__init__()
        self.heat = [[0] * N for _ in range(N)]
        self.last = 0

    def frame(self, t, buf):
        if t - self.last >= 0.05 / max(self.params["speed"], 0.1):
            self.last = t
            h = self.heat
            cool = self.params["cooling"]
            for x in range(N):
                for y in range(N):
                    h[y][x] = max(0, h[y][x] - random.randint(0, cool * 10 // N + 2))
                for y in range(N - 1):  # heat rises (row 0 is the top)
                    h[y][x] = (h[y + 1][x] + h[y + 1][x] + h[min(N - 1, y + 2)][x]) // 3
                if random.randint(0, 255) < 160:
                    h[N - 1][x] = min(255, h[N - 1][x] + random.randint(120, 255))
        for y in range(N):
            for x in range(N):
                v = self.heat[y][x]
                if v < 85:
                    c = (v * 3, 0, 0)
                elif v < 170:
                    c = (255, (v - 85) * 3, 0)
                else:
                    c = (255, 255, (v - 170) * 3)
                buf[y * N + x] = c


class Sparkle(Scene):
    name = "Sparkle"
    params = {"speed": 1.0, "color": 0xFFFFFF, "background": 0x100018}

    def __init__(self):
        super().__init__()
        self.level = [0.0] * (N * N)
        self.last = 0

    def frame(self, t, buf):
        dt = min(0.2, t - self.last)
        self.last = t
        sp = self.params["speed"]
        for _ in range(int(1 + 3 * sp)):
            if random.random() < 0.5 * sp:
                self.level[random.randrange(N * N)] = 1.0
        c = self.params["color"]
        bg = self.params["background"]
        fg = ((c >> 16) & 255, (c >> 8) & 255, c & 255)
        bgc = ((bg >> 16) & 255, (bg >> 8) & 255, bg & 255)
        for i in range(N * N):
            k = self.level[i]
            buf[i] = (int(bgc[0] + (fg[0] - bgc[0]) * k), int(bgc[1] + (fg[1] - bgc[1]) * k),
                      int(bgc[2] + (fg[2] - bgc[2]) * k))
            self.level[i] = max(0.0, k - dt * 2.5 * sp)


class Rain(Scene):
    name = "Rain"
    params = {"speed": 1.0, "color": 0x00FF40}

    def __init__(self):
        super().__init__()
        self.drops = [random.uniform(-8, 8) for _ in range(N)]
        self.last = 0

    def frame(self, t, buf):
        dt = min(0.2, t - self.last)
        self.last = t
        c = self.params["color"]
        col = ((c >> 16) & 255, (c >> 8) & 255, c & 255)
        for i in range(N * N):
            buf[i] = (0, 0, 0)
        for x in range(N):
            self.drops[x] += dt * 9 * self.params["speed"] * (0.6 + 0.4 * ((x * 7) % 3))
            if self.drops[x] > N + 4:
                self.drops[x] = random.uniform(-6, 0)
            head = int(self.drops[x])
            for tail in range(4):
                y = head - tail
                if 0 <= y < N:
                    buf[y * N + x] = scale(col, (1.0, 0.45, 0.2, 0.08)[tail])


class Breathe(Scene):
    name = "Breathe"
    params = {"speed": 1.0, "color": 0xFF2080}

    def frame(self, t, buf):
        c = self.params["color"]
        k = 0.08 + 0.92 * SIN[int(t * 50 * self.params["speed"]) & 255] / 255
        px = (int(((c >> 16) & 255) * k), int(((c >> 8) & 255) * k), int((c & 255) * k))
        for i in range(N * N):
            buf[i] = px


# 5x7 font, one byte per column, bit 0 = top row
FONT = {
    " ": b"\x00\x00\x00\x00\x00", "!": b"\x00\x00\x5f\x00\x00", ".": b"\x00\x60\x60\x00\x00",
    "-": b"\x08\x08\x08\x08\x08", "?": b"\x02\x01\x59\x09\x06", "<": b"\x0c\x1e\x3c\x1e\x0c",
    "0": b"\x3e\x51\x49\x45\x3e", "1": b"\x00\x42\x7f\x40\x00", "2": b"\x72\x49\x49\x49\x46",
    "3": b"\x21\x41\x49\x4d\x33", "4": b"\x18\x14\x12\x7f\x10", "5": b"\x27\x45\x45\x45\x39",
    "6": b"\x3c\x4a\x49\x49\x31", "7": b"\x41\x21\x11\x09\x07", "8": b"\x36\x49\x49\x49\x36",
    "9": b"\x46\x49\x49\x29\x1e", "A": b"\x7c\x12\x11\x12\x7c", "B": b"\x7f\x49\x49\x49\x36",
    "C": b"\x3e\x41\x41\x41\x22", "D": b"\x7f\x41\x41\x41\x3e", "E": b"\x7f\x49\x49\x49\x41",
    "F": b"\x7f\x09\x09\x09\x01", "G": b"\x3e\x41\x41\x51\x73", "H": b"\x7f\x08\x08\x08\x7f",
    "I": b"\x00\x41\x7f\x41\x00", "J": b"\x20\x40\x41\x3f\x01", "K": b"\x7f\x08\x14\x22\x41",
    "L": b"\x7f\x40\x40\x40\x40", "M": b"\x7f\x02\x1c\x02\x7f", "N": b"\x7f\x04\x08\x10\x7f",
    "O": b"\x3e\x41\x41\x41\x3e", "P": b"\x7f\x09\x09\x09\x06", "Q": b"\x3e\x41\x51\x21\x5e",
    "R": b"\x7f\x09\x19\x29\x46", "S": b"\x26\x49\x49\x49\x32", "T": b"\x03\x01\x7f\x01\x03",
    "U": b"\x3f\x40\x40\x40\x3f", "V": b"\x1f\x20\x40\x20\x1f", "W": b"\x3f\x40\x38\x40\x3f",
    "X": b"\x63\x14\x08\x14\x63", "Y": b"\x03\x04\x78\x04\x03", "Z": b"\x61\x59\x49\x4d\x43",
}


class Text(Scene):
    name = "Text"
    # "<" draws a heart
    params = {"speed": 1.0, "text": "HELLO <", "color": 0xFF40A0, "rainbow": True}

    def __init__(self):
        super().__init__()
        self._text = None

    def columns(self):
        if self._text != self.params["text"]:
            self._text = self.params["text"]
            cols = [0] * N  # start off-screen right
            for ch in self._text.upper():
                cols += list(FONT.get(ch, FONT["?"])) + [0]
            self._cols = cols + [0] * 2
        return self._cols

    def frame(self, t, buf):
        cols = self.columns()
        off = int(t * 8 * self.params["speed"]) % len(cols)
        c = self.params["color"]
        col = ((c >> 16) & 255, (c >> 8) & 255, c & 255)
        for x in range(N):
            bits = cols[(off + x) % len(cols)]
            color = wheel(int(t * 40) + x * 10) if self.params["rainbow"] else col
            for y in range(N):
                buf[y * N + x] = color if (bits >> y) & 1 else (0, 0, 0)


CLASSIC = [Rainbow, Plasma, Fire, Sparkle, Rain, Breathe, Text]


# ---------- Mystic set: soft sound-healing -> sacred geometry -> cyberpunk ----------
# Each scene sets `gamma` for the output curve in apps/lightshow.py: low (1.6) gives a
# soft, glowing wash; higher (2.2) gives deep blacks and punch. The list runs from soft
# and glowy to crisp and dynamic.

C = (N - 1) / 2
DIST = [hyp(i % N - C, i // N - C) for i in range(N * N)]
ANG = [math.atan2(i // N - C, i % N - C) for i in range(N * N)]


def hsv(h, s, v):
    h = (h % 1.0) * 6
    i = int(h)
    f = h - i
    v = max(0.0, min(1.0, v))
    p, q, r = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    rgb = ((v, r, p), (q, v, p), (p, v, r), (p, q, v), (r, p, v), (v, p, q))[i % 6]
    return (int(rgb[0] * 255), int(rgb[1] * 255), int(rgb[2] * 255))


def clamp01(v):
    return 0.0 if v < 0 else 1.0 if v > 1 else v


def mix(c1, c2, k):
    return (int(c1[0] + (c2[0] - c1[0]) * k), int(c1[1] + (c2[1] - c1[1]) * k),
            int(c1[2] + (c2[2] - c1[2]) * k))


class SingingBowl(Scene):
    """Slow ripples from the centre, drifting between blue and violet."""
    name = "Singing Bowl"
    params = {"speed": 1.0}
    gamma = 1.6

    def frame(self, t, buf):
        sp = self.params["speed"]
        hue = 0.68 + 0.09 * math.sin(t * 0.06 * sp)  # blue 0.6 <-> violet 0.77, over ~100 s
        for i in range(N * N):
            d = DIST[i]
            w = 0.5 + 0.5 * math.sin(d * 1.4 - t * 0.9 * sp)
            buf[i] = hsv(hue + d * 0.012, 0.75, 0.14 + 0.86 * w * w)


class SoftRain(Scene):
    """Sparse, slow drops drifting down, twinkling as they fall."""
    name = "Soft Rain"
    params = {"speed": 1.0, "drops": 8}
    gamma = 1.8

    def __init__(self):
        super().__init__()
        self.drops = []
        self.last = 0

    def frame(self, t, buf):
        dt = min(0.2, max(0.0, t - self.last))
        self.last = t
        sp = self.params["speed"]
        if not self.drops:  # start already spread down the panel so it never bunches up
            for n in range(self.params["drops"]):
                self.drops.append(self.new_drop(N - n * (N + 2) / self.params["drops"]))
        night = hsv(0.63, 0.9, 0.07)
        for i in range(N * N):
            buf[i] = night
        for n, d in enumerate(self.drops):
            d[1] += dt * 1.5 * sp * d[2]
            if d[1] >= N + 1:
                d[:] = self.new_drop(-random.uniform(0, 1.5))
            y = int(d[1])
            twinkle = d[4] * (0.8 + 0.2 * math.sin(t * 7 + n * 1.9))
            if 0 <= y < N:
                buf[y * N + d[0]] = hsv(d[3], 0.35, twinkle)
            if 0 <= y - 1 < N:
                buf[(y - 1) * N + d[0]] = hsv(d[3], 0.75, 0.3 * twinkle)

    def new_drop(self, y):
        used = [d[0] for d in self.drops]
        free = [x for x in range(N) if x not in used] or list(range(N))
        # column, y, speed, hue, brightness (each drop its own, from faint to bright)
        return [random.choice(free), y, random.uniform(0.85, 1.1), random.uniform(0.56, 0.66),
                random.uniform(0.3, 1.0)]


class Prana(Scene):
    """Breathing guide: inhale 4 s (teal circle grows), hold 3 s, exhale 5 s (rose circle
    shrinks), rest 2 s (a faint ember)."""
    name = "Prana Breath"
    params = {"speed": 1.0}
    gamma = 1.6

    def frame(self, t, buf):
        cyc = (t * self.params["speed"]) % 14
        if cyc < 4:
            k = cyc / 4
            r, hue, v = 0.6 + 4.2 * k * k * (3 - 2 * k), 0.47, 0.65
        elif cyc < 7:
            r, hue, v = 4.8, 0.47 + 0.06 * (cyc - 4) / 3, 0.55 + 0.06 * math.sin((cyc - 4) * 3)
        elif cyc < 12:
            k = (cyc - 7) / 5
            r, hue, v = 4.8 - 4.2 * k * k * (3 - 2 * k), 0.95, 0.6
        else:
            r, hue, v = 0.9, 0.98, 0.25
        for i in range(N * N):
            edge = clamp01((r - DIST[i]) / 1.2 + 0.5)  # clear edge with a soft halo
            buf[i] = hsv(hue, 0.85, 0.05 + v * edge * (1 - 0.35 * DIST[i] / 5))


class SeedOfLife(Scene):
    """Seven thin circles, the outer six slowly turning around a bright centre."""
    name = "Seed of Life"
    params = {"speed": 1.0, "radius": 1.75}
    gamma = 1.9

    def frame(self, t, buf):
        sp = self.params["speed"]
        r = self.params["radius"]
        a0 = t * 0.2 * sp
        centres = [(0.0, 0.0)] + [(r * math.cos(a0 + k * 1.0472), r * math.sin(a0 + k * 1.0472)) for k in range(6)]
        for i in range(N * N):
            x, y = i % N - C, i // N - C
            best, hue = 0.0, 0.6
            for n, (cx, cy) in enumerate(centres):
                line = clamp01(1 - abs(hyp(x - cx, y - cy) - r) * 3.4)
                if line > best:
                    best, hue = line, 0.5 + n * 0.045  # cyan through blue to violet
            core = clamp01(1 - DIST[i] * 1.8)
            buf[i] = mix(hsv(hue + 0.03 * math.sin(t * 0.5), 0.95, 0.07 + 0.93 * best), (255, 255, 255), core)


class Lotus(Scene):
    """An eight-petal lotus opening and closing, pink petals around a golden heart."""
    name = "Lotus Bloom"
    params = {"speed": 1.0}
    gamma = 2.0

    def frame(self, t, buf):
        sp = self.params["speed"]
        bloom = 0.5 - 0.5 * math.cos(t * 0.5 * sp)  # 0 closed .. 1 open
        size = 1.6 + 2.6 * bloom
        rot = t * 0.15 * sp
        for i in range(N * N):
            d = DIST[i]
            petal = size * (0.45 + 0.55 * abs(math.cos(4 * (ANG[i] - rot))))
            if d < 0.95:
                buf[i] = hsv(0.12, 0.75, 0.9)  # golden centre
            elif d < petal:
                k = d / petal
                buf[i] = hsv(0.93 + 0.04 * k, 0.45 + 0.5 * k, 0.9 - 0.45 * k)
            else:
                buf[i] = (14, 0, 10)  # faint rose glow around the flower


class GoldenSpiral(Scene):
    """Spiral arms unwinding from the centre, in honey and amber."""
    name = "Golden Spiral"
    params = {"speed": 1.0, "arms": 2}
    gamma = 2.1

    def frame(self, t, buf):
        sp = self.params["speed"]
        arms = self.params["arms"]
        for i in range(N * N):
            d = DIST[i] + 0.3
            v = 0.5 + 0.5 * math.cos(arms * ANG[i] - math.log(d) * 4.2 + t * 1.5 * sp)
            v = v ** 3
            buf[i] = hsv(0.08 + 0.05 * v, 0.95, v * min(1.0, d / 1.2))


class Metatron(Scene):
    """A star of two crisp triangles turning in opposite directions: gold and cyan."""
    name = "Metatron"
    params = {"speed": 1.0, "radius": 2.0}
    gamma = 2.2

    def frame(self, t, buf):
        sp = self.params["speed"]
        r = self.params["radius"]
        tris = []
        for a0 in (t * 0.25 * sp - math.pi / 2, -t * 0.25 * sp + math.pi / 2):
            tris.append([(math.cos(a0 + k * 2.0944), math.sin(a0 + k * 2.0944)) for k in range(3)])
        colors = ((255, 170, 0), (0, 200, 255))
        for i in range(N * N):
            x, y = i % N - C, i // N - C
            out = (0, 0, 0)
            for tri, col in zip(tris, colors):
                edge = max(x * nx + y * ny for nx, ny in tri)
                line = clamp01(1 - abs(edge - r) * 2.2)
                if line > 0:
                    out = (max(out[0], int(col[0] * line)), max(out[1], int(col[1] * line)),
                           max(out[2], int(col[2] * line)))
            if DIST[i] < 0.8:
                out = (255, 255, 255)
            buf[i] = out


def _root(i, t):  # grounding squares sinking inward, slow and heavy
    x, y = i % N - C, i // N - C
    return (0.5 + 0.5 * math.cos(max(abs(x), abs(y)) * 1.9 + t * 1.2)) ** 2


def _sacral(i, t):  # flowing water
    x, y = i % N, i // N
    return (0.5 + 0.5 * math.sin(x * 0.9 + math.sin(y * 0.8 + t * 0.9) * 1.6 + t * 1.4)) ** 2


def _solar(i, t):  # a turning sun
    rays = max(0.0, math.cos(8 * ANG[i] - t * 1.5)) ** 2
    return min(1.0, rays * 0.8 + clamp01(1.8 - DIST[i]))


def _heart(i, t):  # a heart that beats twice, then rests
    beat = t % 1.6
    s = 1.0 + (0.18 if beat < 0.15 or 0.3 < beat < 0.45 else 0.0)
    x = (i % N - C) / (2.6 * s)
    y = -(i // N - C - 0.3) / (2.6 * s)
    v = (x * x + y * y - 1) ** 3 - x * x * y * y * y
    return 1.0 if v <= 0 else 0.0


def _throat(i, t):  # sound waves rolling outward
    x, y = abs(i % N - C), abs(i // N - C)
    return (0.5 + 0.5 * math.sin(x * 1.7 - t * 3)) ** 2 * clamp01(1.3 - y / 3.5)


def _brow(i, t):  # an iris opening and closing around a dark pupil
    r = 1.6 + 0.9 * math.sin(t * 1.1)
    return clamp01(1 - abs(DIST[i] - r) * 1.6) + (0.0 if DIST[i] > 0.8 else 0.3)


def _crown(i, t):  # spiralling light, with sparkles
    v = (0.5 + 0.5 * math.cos(3 * ANG[i] + DIST[i] * 1.4 - t * 1.6)) ** 3
    return 1.5 if random.random() < 0.025 else v


CHAKRAS = (
    ("Root", (255, 0, 0), _root), ("Sacral", (255, 60, 0), _sacral),
    ("Solar", (255, 170, 0), _solar), ("Heart", (0, 255, 50), _heart),
    ("Throat", (0, 110, 255), _throat), ("Third Eye", (70, 0, 255), _brow),
    ("Crown", (190, 0, 255), _crown),
)


class Chakra(Scene):
    """Rises through the seven chakras from root to crown, each with its own motion."""
    name = "Chakra"
    params = {"speed": 1.0, "seconds_each": 9}
    gamma = 2.2

    def frame(self, t, buf):
        each = self.params["seconds_each"]
        pos = (t * self.params["speed"]) / each
        k = int(pos) % 7
        fade = clamp01((pos - int(pos) - 0.85) / 0.15)  # crossfade over the last 15%
        a_name, a_col, a_fn = CHAKRAS[k]
        b_name, b_col, b_fn = CHAKRAS[(k + 1) % 7]
        self.label = a_name
        for i in range(N * N):
            va = a_fn(i, t)
            ca = (min(255, int(a_col[0] * va)), min(255, int(a_col[1] * va)), min(255, int(a_col[2] * va)))
            if va > 1.2:
                ca = (255, 230, 255)  # crown sparkle
            if fade:
                vb = b_fn(i, t)
                cb = (min(255, int(b_col[0] * vb)), min(255, int(b_col[1] * vb)), min(255, int(b_col[2] * vb)))
                ca = mix(ca, cb, fade)
            buf[i] = ca


class ThirdEye(Scene):
    """Twelve sharp rays turning around a pulsing white-indigo centre."""
    name = "Third Eye"
    params = {"speed": 1.0}
    gamma = 2.1

    def frame(self, t, buf):
        sp = self.params["speed"]
        pulse = 0.5 + 0.5 * math.sin(t * 2.2 * sp)
        for i in range(N * N):
            d = DIST[i]
            rays = max(0.0, math.cos(6 * ANG[i] - t * 1.4 * sp)) ** 3
            core = clamp01(1.3 + pulse * 0.8 - d)
            ray = hsv(0.73, 1.0, rays * clamp01(0.4 + d / 3))
            buf[i] = mix(ray, mix((70, 0, 255), (255, 255, 255), pulse * 0.6), core)


class DataStream(Scene):
    """Matrix code rising up the panel: bright green-white heads, fading green trails."""
    name = "Data Stream"
    params = {"speed": 1.0}
    gamma = 2.1

    def __init__(self):
        super().__init__()
        self.drops = [random.uniform(-8, 8) for _ in range(N)]
        self.rates = [random.uniform(0.6, 1.3) for _ in range(N)]
        self.last = 0

    def frame(self, t, buf):
        dt = min(0.2, max(0.0, t - self.last))
        self.last = t
        for i in range(N * N):
            buf[i] = (0, 0, 0)
        for x in range(N):
            self.drops[x] += dt * 7 * self.params["speed"] * self.rates[x]
            if self.drops[x] > N + 6:
                self.drops[x] = random.uniform(-6, 0)
                self.rates[x] = random.uniform(0.6, 1.3)
            head = int(self.drops[x])
            for tail in range(6):
                y = head - tail
                if 0 <= y < N:
                    if tail == 0:
                        c = (180, 255, 180)
                    else:
                        g = (230, 150, 90, 45, 18)[tail - 1]
                        c = (0, g if random.random() > 0.06 else g // 3, 0)  # flicker like glyphs
                    buf[(N - 1 - y) * N + x] = c  # drawn upside down, so it streams upward


class Synthwave(Scene):
    """Retro sun with scrolling stripes over a magenta horizon and a moving floor line."""
    name = "Synthwave"
    params = {"speed": 1.2}
    gamma = 2.0

    def frame(self, t, buf):
        sp = self.params["speed"]
        scroll = (t * 0.7 * sp) % 1.0
        for y in range(N):
            for x in range(N):
                i = y * N + x
                if y <= 4:  # sky and sun
                    if hyp((x - C) * 1.25, y - 4.8) < 4.3:
                        gap = y >= 3 and ((y + scroll) % 2) < 0.6  # stripes slide down the sun
                        buf[i] = (0, 0, 0) if gap else mix((255, 220, 0), (255, 0, 120), y / 4)
                    else:
                        buf[i] = (25, 0, 45)
                elif y == 5:
                    buf[i] = (255, 0, 180)  # horizon
                else:
                    near = (t * 1.4 * sp) % 1.0 > 0.5
                    line = (y == 7) if near else (y == 6)
                    vert = x in ((2, 5) if y == 6 else (0, 3, 4, 7))
                    buf[i] = (0, 200, 255) if line else (0, 90, 140) if vert else (15, 0, 30)


# Shown in this order in the app (soft -> cyberpunk). The classic scenes above stay
# available for the web editor.
ALL = [SingingBowl, SoftRain, Prana, SeedOfLife, Lotus, GoldenSpiral,
       Metatron, Chakra, ThirdEye, DataStream, Synthwave]
