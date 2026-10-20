# Bluetooth scanner: what's broadcasting nearby, grouped by maker, plus a radar plot.
# Tap BOOT to switch views, double-tap to go back.
import time
import displayio
import _bleio
import ui

FORGET_AFTER = 30  # seconds without hearing a device before it drops off
ROWS = 6
DOTS = 28

APPLE_TYPES = {
    0x02: "iBeacon",
    0x05: "AirDrop",
    0x07: "AirPods",
    0x09: "AirPlay",
    0x0C: "Apple Handoff",
    0x0F: "Apple device",
    0x10: "Apple device",
    0x12: "Find My tag",
}
COMPANIES = {0x0006: "Windows", 0x0075: "Samsung", 0x00E0: "Google", 0x0087: "Garmin"}
SERVICES = {0xFEED: "Tile tracker", 0xFEEC: "Tile tracker", 0xFD5A: "SmartTag",
            0xFE9F: "Google", 0xFD6F: "Exposure notif"}
TRACKERS = ("Find My tag", "Tile tracker", "SmartTag")

GROUPS = (
    ("Apple", ui.FG),
    ("Trackers", ui.WARN),
    ("Windows", ui.ACCENT),
    ("Samsung", ui.PURPLE),
    ("Google", ui.GOOD),
    ("Other", ui.DIM),
)


def group_of(kind):
    if kind in TRACKERS:
        return "Trackers"
    if kind in APPLE_TYPES.values() or kind == "Apple":
        return "Apple"
    if kind in ("Windows", "Samsung", "Google"):
        return kind
    return "Other"


def color_of(kind):
    g = group_of(kind)
    for name, color in GROUPS:
        if name == g:
            return color
    return ui.DIM


def parse(adv):
    """Return (name, kind) from raw advertisement bytes."""
    name = kind = None
    i = 0
    n = len(adv)
    while i + 1 < n:
        ln = adv[i]
        if ln == 0 or i + 1 + ln > n:
            break
        t = adv[i + 1]
        data = adv[i + 2: i + 1 + ln]
        if t in (0x08, 0x09) and data:
            try:
                name = bytes(data).decode("utf-8")
            except UnicodeError:
                pass
        elif t == 0xFF and len(data) >= 2:
            company = data[0] | (data[1] << 8)
            if company == 0x004C:
                kind = APPLE_TYPES.get(data[2], "Apple") if len(data) > 2 else "Apple"
            elif kind is None:
                kind = COMPANIES.get(company)
        elif t in (0x02, 0x03, 0x16) and len(data) >= 2:
            uuid = data[0] | (data[1] << 8)
            kind = SERVICES.get(uuid, kind)
        i += 1 + ln
    return name, kind


def angle_for(addr):
    h = 0
    for b in addr:
        h = (h * 31 + b) & 0xFFFF
    return h


class ListView:
    title = "BT NEARBY"

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

    def update(self, devices):
        ranked = sorted(devices.items(), key=lambda kv: -kv[1][0])
        for i, (bars, name, meta) in enumerate(self.rows):
            if i < len(ranked):
                addr, (rssi, dev_name, kind, _) = ranked[i]
                bars.set(rssi)
                text = dev_name or kind or "%02X:%02X:%02X" % (addr[2], addr[1], addr[0])
                name.text = ui.clip(text, 22)
                name.color = color_of(kind) if (kind or dev_name) else ui.DIM
                # Named devices show their type on the right; everything else shows signal
                meta.text = ui.clip(kind, 9) if (dev_name and kind) else "%d" % rssi
            else:
                bars.set(None)
                name.text = "listening..." if not devices and i == 0 else ""
                meta.text = ""


class SummaryView:
    title = "BT SUMMARY"

    def __init__(self):
        self.g = displayio.Group()
        self.total = ui.lbl("0", 44, 70, ui.PURPLE, scale=4, anchor=(0.5, 0.5))
        self.g.append(self.total)
        self.g.append(ui.lbl("devices", 44, 102, ui.DIM, anchor=(0.5, 0.5)))
        self.bars = []
        self.nums = []
        for i, (name, color) in enumerate(GROUPS):
            y = 26 + i * 18
            self.g.append(ui.lbl(name, 92, y + 4, ui.DIM))
            self.g.append(ui.rect(150, y, 60, 9, ui.PANEL))
            bar = ui.rect(150, y, 1, 9, color)
            self.g.append(bar)
            self.bars.append(bar)
            num = ui.lbl("0", ui.W - 6, y + 4, color, anchor=(1.0, 0.5))
            self.g.append(num)
            self.nums.append(num)

    def update(self, devices):
        counts = {name: 0 for name, _ in GROUPS}
        for rssi, _, kind, _ in devices.values():
            counts[group_of(kind)] += 1
        total = len(devices)
        self.total.text = str(total)
        top = max(max(counts.values()), 1)
        for i, (name, _) in enumerate(GROUPS):
            self.bars[i].width = max(1, int(60 * counts[name] / top))
            self.nums[i].text = str(counts[name])


class RadarView:
    title = "BT RADAR"
    CX, CY, R = ui.W // 2, 77, 56

    def __init__(self):
        self.g = displayio.Group()
        for r, c in ((self.R, ui.PANEL), (self.R * 2 // 3, ui.BG), (self.R * 2 // 3 - 1, ui.PANEL),
                     (self.R // 3, ui.BG), (self.R // 3 - 1, ui.PANEL)):
            self.g.append(ui.circle(self.CX, self.CY, r, c))
        self.g.append(ui.rect(self.CX - self.R, self.CY, self.R * 2, 1, ui.LINE))
        self.g.append(ui.rect(self.CX, self.CY - self.R, 1, self.R * 2, ui.LINE))
        self.g.append(ui.circle(self.CX, self.CY, 3, ui.ACCENT))
        self.g.append(ui.lbl("near", 6, 124, ui.DIM, anchor=(0.0, 0.5)))
        self.g.append(ui.lbl("far", ui.W - 6, 124, ui.DIM, anchor=(1.0, 0.5)))
        self.dots = [ui.circle(-20, -20, 3, ui.FG) for _ in range(DOTS)]
        for d in self.dots:
            self.g.append(d)
        # Precomputed unit circle (no math.cos in a hurry)
        import math
        self.unit = [(math.cos(a * math.pi / 32), math.sin(a * math.pi / 32)) for a in range(64)]

    def update(self, devices):
        ranked = sorted(devices.items(), key=lambda kv: -kv[1][0])[:DOTS]
        for i, d in enumerate(self.dots):
            if i < len(ranked):
                addr, (rssi, _, kind, _) = ranked[i]
                # -35 dBm sits next to you, -100 at the outer ring
                dist = min(1.0, max(0.08, (-35 - rssi) / 65))
                ux, uy = self.unit[angle_for(addr) % 64]
                d.x = int(self.CX + ux * dist * self.R)
                d.y = int(self.CY + uy * dist * self.R)
                d.pixel_shader = ui.pal(color_of(kind))
                d.radius = 4 if group_of(kind) == "Trackers" else 3
            else:
                d.x = d.y = -20


def run(btn, px):
    g = ui.screen()
    title, status = ui.header(g, ListView.title, ui.PURPLE)
    adapter = _bleio.adapter
    if not adapter.enabled:
        adapter.enabled = True
    views = [ListView(), SummaryView(), RadarView()]
    current = 0
    g.append(views[0].g)
    devices = ui.state.setdefault("ble_devices", {})  # addr -> (rssi, name, kind, last_seen)

    while True:
        status.text = "%d nearby" % len(devices)
        try:
            for entry in adapter.start_scan(timeout=1.2, minimum_rssi=-100, active=True,
                                            buffer_size=1536):
                addr = bytes(entry.address.address_bytes)
                name, kind = parse(entry.advertisement_bytes)
                old = devices.get(addr)
                rssi = entry.rssi
                if old:
                    rssi = (old[0] * 2 + rssi) // 3  # smooth the jitter
                    name = name or old[1]
                    kind = kind or old[2]
                devices[addr] = (rssi, name, kind, time.monotonic())
                e = btn.poll()
                if e:
                    break
            else:
                e = None
        finally:
            adapter.stop_scan()

        now = time.monotonic()
        for addr in [a for a, v in devices.items() if now - v[3] > FORGET_AFTER]:
            del devices[addr]

        trackers = sum(1 for v in devices.values() if group_of(v[2]) == "Trackers")
        status.text = "%d nearby" % len(devices) + ("  %d tag" % trackers if trackers else "")
        px.color(ui.WARN if trackers else ui.blend(ui.PANEL, ui.PURPLE, len(devices) / 30))
        views[current].update(devices)

        if e is None:
            e = btn.wait(0.3)
        if e in ("long", "double"):
            return
        if e == "short":
            g.remove(views[current].g)
            current = (current + 1) % len(views)
            g.append(views[current].g)
            title.text = views[current].title
            views[current].update(devices)
