# System: vitals (temperature, CPU, memory, storage), radios, and settings.
# Tap BOOT for the next page. On Settings: tap moves down, hold changes the value.
# Double-tap to go back.
import gc
import os
import time
import board
import displayio
import microcontroller
import ui

SETTINGS = [
    # (label, key, choices, how to show a choice)
    ("Screen", "screen", (0.25, 0.5, 0.75, 1.0), lambda v: "%d%%" % (v * 100)),
    ("LED panel", "led_level", (0, 1, 2, 3, 4), lambda v: "%d of 5" % (v + 1)),
    ("Status light", "status_led", (0, 1, 2, 3), lambda v: ("off", "low", "medium", "high")[v]),
    ("Wi-Fi", "wifi", (True, False), lambda v: "on" if v else "off"),
    ("Units", "units", ("imperial", "metric"), lambda v: v),
    ("Slide time", "slide_seconds", (0.5, 1.0, 2.0, 4.0), lambda v: "%gs" % v),
    ("Alt log every", "log_every", (5, 30, 60, 300), lambda v: "%ds" % v if v < 60 else "%dm" % (v // 60)),
]


def mood(c):
    if c < 45:
        return "cool & cozy", ui.GOOD
    if c < 60:
        return "warm & happy", ui.WARN
    return "toasty! air pls", ui.BAD


class Pill:
    """Rounded progress bar. Built once; set() only moves the fill's end."""

    def __init__(self, g, x, y, w, color):
        self.x, self.w = x, w
        g.append(ui.rect(x + 3, y, w - 6, 7, ui.PANEL))
        g.append(ui.circle(x + 3, y + 3, 3, ui.PANEL))
        g.append(ui.circle(x + w - 4, y + 3, 3, ui.PANEL))
        g.append(ui.circle(x + 3, y + 3, 3, color))
        self.fill = ui.rect(x + 3, y, 1, 7, color)
        self.cap = ui.circle(x + 3, y + 3, 3, color)
        g.append(self.fill)
        g.append(self.cap)

    def set(self, frac):
        fw = max(6, int(self.w * min(1.0, frac)))
        if self.fill.width != max(1, fw - 6):
            self.fill.width = max(1, fw - 6)
            self.cap.x = self.x + fw - 4


def row(g, y, label, color=ui.FG):
    """A label and an empty value; returns the value label to update later."""
    g.append(ui.lbl(label, 8, y, ui.DIM))
    v = ui.lbl("", 82, y, color)
    g.append(v)
    return v


def put(lbl, text, color=None):
    # Only touch the screen when something changed; redrawing everything is what flickered
    if lbl.text != text:
        lbl.text = text
    if color is not None and lbl.color != color:
        lbl.color = color


def uptime():
    s = int(time.monotonic())
    return "%d:%02d:%02d" % (s // 3600, s // 60 % 60, s % 60)


_sensor = []


def sensor_temp():
    try:
        if not _sensor:
            import adafruit_bmp280
            _sensor.append(adafruit_bmp280.Adafruit_BMP280_I2C(board.I2C(), address=0x77))
        return _sensor[0].temperature
    except (ValueError, RuntimeError, OSError, ImportError):
        return None


def vitals(body):
    """Draw the Vitals page once; returns a function that refreshes its numbers."""
    chip = row(body, 27, "Chip")
    chip_mood = ui.lbl("", ui.W - 8, 27, ui.FG, anchor=(1.0, 0.5))
    body.append(chip_mood)
    put(row(body, 43, "CPU"), "%d MHz" % (microcontroller.cpu.frequency // 1000000))
    mem = row(body, 59, "Memory")
    mem_bar = Pill(body, 170, 56, 62, ui.PURPLE)
    disk = row(body, 75, "Storage")
    disk_bar = Pill(body, 170, 72, 62, ui.PINK)
    up = row(body, 91, "Uptime")
    put(row(body, 107, "Firmware"), "CP " + os.uname().release)
    sensor = row(body, 123, "Sensor", ui.DIM)

    def update():
        c = microcontroller.cpu.temperature
        text, color = mood(c)
        put(chip, ui.temp_text(c), color)
        put(chip_mood, text, color)
        gc.collect()
        free, used = gc.mem_free(), gc.mem_alloc()
        put(mem, "%d KB free" % (free // 1024))
        mem_bar.set(used / (used + free))
        st = os.statvfs("/")
        total, avail = st[0] * st[2] // 1024, st[0] * st[3] // 1024
        put(disk, "%d KB free" % avail)
        disk_bar.set((total - avail) / total)
        put(up, uptime())
        sc = sensor_temp()
        put(sensor, "" if sc is None else ui.temp_text(sc) + " (LEDs warm it)")

    update()
    return update


def radios(body):
    """Draw the Radios page once; returns a function that refreshes it."""
    import wifi
    import _bleio
    r = wifi.radio
    w = row(body, 27, "Wi-Fi")
    bt = row(body, 43, "Bluetooth")
    put(row(body, 59, "MAC"), ":".join("%02X" % b for b in r.mac_address))
    i2c_v = row(body, 75, "I2C")
    put(row(body, 91, "LED pin"), os.getenv("LS_PIN") or "D5")
    reason = str(microcontroller.cpu.reset_reason).split(".")[-1].replace("_", " ").lower()
    put(row(body, 107, "Last reset"), reason)
    put(row(body, 123, "Panel"), "http://%s" % r.ipv4_address if r.connected else "needs Wi-Fi", ui.ACCENT)

    def update():
        if not r.enabled:
            put(w, "off", ui.DIM)
        elif r.connected:
            put(w, "%s  %d dBm" % (ui.clip(str(r.ap_info.ssid), 14), r.ap_info.rssi), ui.GOOD)
        else:
            put(w, "on, not joined", ui.WARN)
        on = _bleio.adapter.enabled
        put(bt, "on" if on else "off", ui.GOOD if on else ui.DIM)
        try:
            i2c = board.I2C()
            while not i2c.try_lock():
                pass
            found = [hex(a) for a in i2c.scan()]
            i2c.unlock()
        except (RuntimeError, ValueError):
            found = []
        put(i2c_v, " ".join(found) or "nothing plugged in")

    update()
    return update


def settings_page(body, sel):
    for i, (label, key, choices, show) in enumerate(SETTINGS):
        y = 22 + i * 15
        if i == sel:
            body.append(ui.rect(8, y, ui.W - 16, 13, ui.PANEL))
            body.append(ui.circle(8, y + 6, 6, ui.PANEL))
            body.append(ui.circle(ui.W - 9, y + 6, 6, ui.PANEL))
            body.append(ui.star(10, y + 6, 4, ui.GOLD))
        body.append(ui.lbl(label, 20, y + 6, ui.FG if i == sel else ui.DIM))
        body.append(ui.lbl(show(ui.settings[key]), ui.W - 12, y + 6, ui.PINK if i == sel else ui.FG,
                           anchor=(1.0, 0.5)))


PAGES = ("VITALS", "RADIOS", "SETTINGS")
HINTS = ("tap: next", "tap: next", "hold: change")


def run(btn, px):
    g = ui.screen()
    title, hint = ui.header(g, "SYSTEM", ui.PINK)
    body = displayio.Group()
    g.append(body)
    page, sel = 0, 0
    redraw = True
    update = None
    last = 0
    while True:
        now = time.monotonic()
        if redraw:
            while len(body):
                body.pop()
            title.text = "SYSTEM  " + PAGES[page].lower()
            hint.text = HINTS[page]
            update = vitals(body) if page == 0 else radios(body) if page == 1 else None
            if page == 2:
                settings_page(body, sel)
            redraw, last = False, now
            px.color(ui.RAINBOW[page * 2])
        elif update is not None and now - last > (1.0 if page == 0 else 3.0):
            update()
            last = now
        e = btn.wait(0.05)
        if e == "double":
            return
        if e == "short":
            if page == 2 and sel < len(SETTINGS) - 1:
                sel += 1
            else:
                page, sel = (page + 1) % len(PAGES), 0
            redraw = True
        elif e == "long" and page == 2:
            label, key, choices, _ = SETTINGS[sel]
            cur = ui.settings[key]
            nxt = choices[(choices.index(cur) + 1) % len(choices)] if cur in choices else choices[0]
            ui.settings[key] = nxt
            ui.save_settings()
            ui.apply_settings(px)
            redraw = True
