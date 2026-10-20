# Wi-Fi Radar: nearby networks with signal bars, and a channel congestion chart.
# Tap BOOT to switch views, double-tap to go back.
import time
import displayio
import wifi
import ui

ROWS = 6
CHANNELS = 13


def scan():
    seen = {}
    wifi.radio.enabled = True
    try:
        for n in wifi.radio.start_scanning_networks():
            key = bytes(n.bssid)
            open_ = wifi.AuthMode.OPEN in n.authmode
            old = seen.get(key)
            if old is None or n.rssi > old[1]:
                seen[key] = (n.ssid, n.rssi, n.channel, open_)
    finally:
        wifi.radio.stop_scanning_networks()
    return sorted(seen.values(), key=lambda t: -t[1])


class ListView:
    title = "WI-FI RADAR"

    def __init__(self):
        self.g = displayio.Group()
        self.rows = []
        for i in range(ROWS):
            y = 20 + i * 19
            bars = ui.Bars(self.g, 6, y + 1)
            name = ui.lbl("", 28, y + 7)
            meta = ui.lbl("", ui.W - 6, y + 7, ui.DIM, anchor=(1.0, 0.5))
            self.g.append(name)
            self.g.append(meta)
            self.rows.append((bars, name, meta))

    def update(self, nets):
        for i, (bars, name, meta) in enumerate(self.rows):
            if i < len(nets):
                ssid, rssi, ch, open_ = nets[i]
                bars.set(rssi)
                name.text = ui.clip(ssid or "(hidden)", 24)
                name.color = ui.WARN if open_ else ui.FG
                meta.text = "ch%d %d" % (ch, rssi)
            else:
                bars.set(None)
                name.text = "" if nets or i else "scanning..."
                meta.text = ""


class ChannelView:
    title = "CHANNELS"
    TOP, BOTTOM = 34, 112
    SLOT = 17

    def __init__(self):
        self.g = displayio.Group()
        self.bars = []
        self.counts = []
        x0 = (ui.W - CHANNELS * self.SLOT) // 2
        self.g.append(ui.rect(x0, self.BOTTOM + 1, CHANNELS * self.SLOT, 1, ui.LINE))
        for c in range(CHANNELS):
            x = x0 + c * self.SLOT + 3
            bar = ui.rect(x, self.BOTTOM, 11, 1, ui.LINE)
            self.g.append(bar)
            self.bars.append(bar)
            cnt = ui.lbl("", x + 6, self.BOTTOM - 8, ui.DIM, anchor=(0.5, 0.5))
            self.g.append(cnt)
            self.counts.append(cnt)
            self.g.append(ui.lbl(str(c + 1), x + 6, self.BOTTOM + 11,
                                 ui.FG if c + 1 in (1, 6, 11) else ui.DIM, anchor=(0.5, 0.5)))
        self.best = ui.lbl("", ui.W // 2, 25, ui.GOOD, anchor=(0.5, 0.5))
        self.g.append(self.best)

    def update(self, nets):
        score = [0.0] * CHANNELS
        count = [0] * CHANNELS
        for _, rssi, ch, _ in nets:
            if 1 <= ch <= CHANNELS:
                # Neighbouring channels overlap, so a network also crowds its neighbours a bit
                w = max(0, rssi + 100) / 50
                for d in (-2, -1, 0, 1, 2):
                    c = ch - 1 + d
                    if 0 <= c < CHANNELS:
                        score[c] += w * (1.0, 0.6, 0.3)[abs(d)]
                count[ch - 1] += 1
        top = max(max(score), 1.0)
        span = self.BOTTOM - self.TOP
        for c in range(CHANNELS):
            h = int(span * score[c] / top)
            self.bars[c].height = max(1, h)
            self.bars[c].y = self.BOTTOM - max(1, h)
            self.bars[c].pixel_shader = ui.pal(ui.blend(ui.GOOD, ui.BAD, score[c] / top))
            self.counts[c].text = str(count[c]) if count[c] else ""
            self.counts[c].anchored_position = (self.counts[c].anchored_position[0],
                                                self.BOTTOM - max(1, h) - 7)
        best = min((1, 6, 11), key=lambda ch: score[ch - 1])
        self.best.text = "least crowded: ch %d" % best if nets else ""


def run(btn, px):
    g = ui.screen()
    title, status = ui.header(g, ListView.title, ui.GOOD)
    views = [ListView(), ChannelView()]
    current = 0
    g.append(views[0].g)
    nets = []
    while True:
        status.text = "scanning"
        px.color(ui.ACCENT)
        try:
            nets = scan()
        except (RuntimeError, OSError, ConnectionError) as err:
            status.text = "scan hiccup, retrying"
            print("scan failed:", err)
            if btn.wait(2) in ("long", "double"):
                return
            continue
        n = len(nets)
        status.text = "%d networks" % n
        px.color(ui.GOOD if n < 8 else ui.WARN if n < 20 else ui.BAD)
        views[current].update(nets)

        e = btn.wait(1.5)
        if e in ("long", "double"):
            return
        if e == "short":
            g.remove(views[current].g)
            current = (current + 1) % len(views)
            g.append(views[current].g)
            title.text = views[current].title
            views[current].update(nets)
