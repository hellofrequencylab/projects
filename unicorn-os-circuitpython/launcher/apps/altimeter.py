# Altimeter: height from the BMP280 pressure sensor, with a log that survives power-off.
# Pages: Live -> Graph -> Stats. Tap BOOT for the next page, double-tap to go back.
# Live: hold to zero the "since zero" reading. Graph: hold twice to start a new log.
# Logs while this app is open, every System > Settings > Alt log every.
import struct
import time
import board
import displayio
import microcontroller
import ui

SEA_LEVEL_HPA = 1013.25  # standard air; absolute height can be off, changes are accurate
NVM = microcontroller.nvm
LOG_AT = 1024
HDR = 8                   # b"AL", count, head, interval (u16 each)
CAP = (len(NVM) - LOG_AT - HDR) // 2
MOUNTAIN = ("......W.....", ".....WWL....", "....WLLLL...", "...LLLLLLL..", "..LLLLPLLLL.",
            ".LLLLPPPLLLL", "GGGGGGGGGGGG")
MOUNTAIN_COLORS = {"W": ui.FG, "L": ui.PURPLE, "P": ui.PINK, "G": ui.GOOD}


# ---------- log in non-volatile memory: altitude in half-metres, int16, ring buffer ----------

def log_header():
    if bytes(NVM[LOG_AT:LOG_AT + 2]) != b"AL":
        return 0, 0, ui.settings["log_every"]
    return struct.unpack("<HHH", bytes(NVM[LOG_AT + 2:LOG_AT + HDR]))


def log_clear(interval):
    NVM[LOG_AT:LOG_AT + HDR] = b"AL" + struct.pack("<HHH", 0, 0, interval)


def log_append(metres):
    count, head, interval = log_header()
    if bytes(NVM[LOG_AT:LOG_AT + 2]) != b"AL":
        log_clear(interval)
    at = LOG_AT + HDR + head * 2
    NVM[at:at + 2] = struct.pack("<h", max(-32768, min(32767, int(metres * 2))))
    NVM[LOG_AT + 2:LOG_AT + HDR] = struct.pack("<HHH", min(count + 1, CAP), (head + 1) % CAP, interval)


def log_read():
    count, head, interval = log_header()
    if not count:
        return [], interval
    raw = bytes(NVM[LOG_AT + HDR:LOG_AT + HDR + CAP * 2])
    start = (head - count) % CAP
    vals = [struct.unpack_from("<h", raw, ((start + i) % CAP) * 2)[0] / 2 for i in range(count)]
    return vals, interval


# ---------- units ----------

def height(m):
    return ("%d" % round(m * 3.28084), "ft") if ui.imperial() else ("%d" % round(m), "m")


def signed(m):
    v, u = height(m)
    return ("+" if not v.startswith("-") else "") + v + " " + u


def span_text(seconds):
    m = int(seconds // 60)
    return "%dh %02dm" % (m // 60, m % 60) if m >= 60 else "%dm" % m if m else "%ds" % seconds


# ---------- pages ----------

def put(lbl, text, color=None):
    # Only touch the screen when something changed; full redraws were what flickered
    if lbl.text != text:
        lbl.text = text
    if color is not None and lbl.color != color:
        lbl.color = color


def live(body):
    """Draw the Live page once; returns update(alt, zero, rate, hpa)."""
    body.append(ui.sprite(MOUNTAIN, MOUNTAIN_COLORS, ui.W - 34, 22, 2))
    big = ui.lbl("", 12, 48, ui.FG, scale=4)
    unit = ui.lbl("", 12, 76, ui.DIM)
    since = ui.lbl("", 12, 94, ui.PINK)
    move = ui.lbl("", 12, 110, ui.DIM)
    press = ui.lbl("", ui.W - 8, 110, ui.DIM, anchor=(1.0, 0.5))
    logline = ui.lbl("", 12, 125, ui.DIM)
    for item in (big, unit, since, move, press, logline):
        body.append(item)

    def update(alt, zero, rate, hpa):
        v, u = height(alt)
        put(big, v)
        put(unit, u + " above sea level")
        put(since, signed(alt - zero) + " since zero")
        if abs(rate) < 0.15:
            put(move, "floating steady", ui.DIM)
        else:
            rv, ru = height(abs(rate) * 60)
            put(move, ("climbing " if rate > 0 else "gliding down ") + rv + " " + ru + "/min",
                ui.GOOD if rate > 0 else ui.ACCENT)
        put(press, "%.1f hPa" % hpa)
        count, _, interval = log_header()
        put(logline, "logging every %ds - %d points" % (interval, count))

    return update


def graph(body, vals, interval):
    if len(vals) < 2:
        body.append(ui.lbl("no log yet, it fills in", ui.W // 2, 60, ui.DIM, anchor=(0.5, 0.5)))
        body.append(ui.lbl("while this app is open", ui.W // 2, 76, ui.DIM, anchor=(0.5, 0.5)))
        return
    cols = 110
    covers = span_text(len(vals) * interval)
    if len(vals) > cols:  # average down to one column per 2 px
        step = len(vals) / cols
        vals = [sum(vals[int(i * step):max(int(i * step) + 1, int((i + 1) * step))]) /
                max(1, int((i + 1) * step) - int(i * step)) for i in range(cols)]
    lo, hi = min(vals), max(vals)
    span = max(hi - lo, 2.0)
    top, bottom = 34, 118
    x0 = ui.W - 8 - len(vals) * 2
    for i, v in enumerate(vals):
        h = 2 + int((bottom - top) * (v - lo) / span)
        color = ui.RAINBOW[int(i * len(ui.RAINBOW) / len(vals))]
        body.append(ui.rect(x0 + i * 2, bottom - h, 2, h, color))
    body.append(ui.lbl("max " + "".join(height(hi)), 8, 26, ui.FG))
    body.append(ui.lbl("min " + "".join(height(lo)), 8, 127, ui.DIM))
    body.append(ui.lbl("last " + covers, ui.W - 8, 127, ui.DIM, anchor=(1.0, 0.5)))


def stats(body, vals, interval):
    if not vals:
        body.append(ui.lbl("nothing logged yet", ui.W // 2, 70, ui.DIM, anchor=(0.5, 0.5)))
        return
    up = down = 0.0
    for a, b in zip(vals, vals[1:]):
        d = b - a
        if d > 0.5:
            up += d
        elif d < -0.5:
            down -= d
    rows = (("Points", "%d of %d" % (len(vals), CAP)), ("Covers", span_text(len(vals) * interval)),
            ("Highest", " ".join(height(max(vals)))), ("Lowest", " ".join(height(min(vals)))),
            ("Climbed", " ".join(height(up))), ("Descended", " ".join(height(down))),
            ("Interval", "%ds" % interval))
    for i, (k, v) in enumerate(rows):
        y = 27 + i * 15
        body.append(ui.lbl(k, 10, y, ui.DIM))
        body.append(ui.lbl(v, ui.W - 10, y, ui.FG if i % 2 else ui.PINK, anchor=(1.0, 0.5)))


PAGES = ("live", "graph", "stats")
HINTS = ("hold: zero", "hold x2: new log", "tap: next")


def run(btn, px):
    g = ui.screen()
    title, hint = ui.header(g, "ALTIMETER", ui.PINK)
    body = displayio.Group()
    g.append(body)
    try:
        import adafruit_bmp280
        sensor = adafruit_bmp280.Adafruit_BMP280_I2C(board.I2C(), address=0x77)
        sensor.overscan_pressure = adafruit_bmp280.OVERSCAN_X16
        sensor.iir_filter = adafruit_bmp280.IIR_FILTER_X16
        sensor.sea_level_pressure = SEA_LEVEL_HPA
    except (ValueError, RuntimeError, OSError, ImportError):
        body.append(ui.lbl("no pressure sensor found", ui.W // 2, 60, ui.BAD, anchor=(0.5, 0.5)))
        body.append(ui.lbl("plug a BMP280 into STEMMA QT", ui.W // 2, 78, ui.DIM, anchor=(0.5, 0.5)))
        while btn.wait(1) != "double":
            pass
        return

    alt = sensor.altitude
    hpa = sensor.pressure
    zero = ui.state.get("alt_zero", alt)
    history = []  # (time, alt) for the climb rate
    page = 0
    confirm_until = 0
    last_draw = last_log = 0
    interval = log_header()[2]
    redraw, drawn_count, update = True, -1, None
    while True:
        now = time.monotonic()
        try:
            alt = alt * 0.8 + sensor.altitude * 0.2  # smooth out sensor jitter
            hpa = sensor.pressure
        except (OSError, RuntimeError):
            pass
        history.append((now, alt))
        while history and now - history[0][0] > 20:
            history.pop(0)
        rate = (history[-1][1] - history[0][1]) / max(1.0, history[-1][0] - history[0][0])
        if now - last_log >= interval:
            log_append(alt)
            last_log = now
        count = log_header()[0]
        if redraw or (page and count != drawn_count):
            # build the page once; Graph and Stats only rebuild when a new point is logged
            while len(body):
                body.pop()
            title.text = "ALTIMETER  " + PAGES[page]
            update = None
            if page == 0:
                update = live(body)
            else:
                vals, iv = log_read()
                (graph if page == 1 else stats)(body, vals, iv)
            drawn_count, redraw = count, False
        if page == 0 and now - last_draw > 0.5:
            update(alt, zero, rate, hpa)
            last_draw = now
        hint_text = "hold again: clear" if now < confirm_until else HINTS[page]
        if hint.text != hint_text:
            hint.text = hint_text
        px.color(ui.GOOD if rate > 0.15 else ui.ACCENT if rate < -0.15 else ui.PURPLE)
        e = btn.wait(0.15)
        if e == "double":
            ui.state["alt_zero"] = zero
            return
        if e == "short":
            page = (page + 1) % len(PAGES)
            redraw, last_draw = True, 0
        elif e == "long":
            if page == 0:
                zero = alt
            elif page == 1:
                if now < confirm_until:
                    interval = ui.settings["log_every"]
                    log_clear(interval)
                    confirm_until = 0
                    redraw = True
                else:
                    confirm_until = now + 4
            last_draw = 0
