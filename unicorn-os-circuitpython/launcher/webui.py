# The unicorn control panel at http://<board ip>/ . It's polled from ui.Button.poll, which
# every app calls constantly, so the page keeps working whichever app is open.
# CircuitPython's own file dashboard moves to port 8080 (CIRCUITPY_WEB_API_PORT).
import gc
import json
import os
import struct
import time
import microcontroller
import ui

APPS = (("menu", "Menu"), ("lightshow", "Light Show"), ("system", "System"), ("altimeter", "Altimeter"),
        ("wifi_radar", "Wi-Fi Radar"), ("ble_scanner", "BT Scanner"), ("slideshow", "Slideshow"),
        ("wifi_setup", "Wi-Fi Setup"))
ALLOWED = {  # settings the page may change, and their valid values
    "screen": (0.25, 0.5, 0.75, 1.0), "led_level": (0, 1, 2, 3, 4), "status_led": (0, 1, 2, 3),
    "units": ("imperial", "metric"), "slide_seconds": (0.5, 1.0, 2.0, 4.0), "log_every": (5, 30, 60, 300),
}
SEA_LEVEL_HPA = 1013.25

_server = None
_last = 0
_sensor = []


def _altitude():
    try:
        if not _sensor:
            import board
            import adafruit_bmp280
            s = adafruit_bmp280.Adafruit_BMP280_I2C(board.I2C(), address=0x77)
            s.sea_level_pressure = SEA_LEVEL_HPA
            _sensor.append(s)
        return _sensor[0].altitude, _sensor[0].pressure
    except (ValueError, RuntimeError, OSError, ImportError):
        return None, None


def _status():
    import wifi
    import scenes
    gc.collect()
    st = os.statvfs("/")
    c = microcontroller.cpu.temperature
    alt, hpa = _altitude()
    r = wifi.radio
    return {
        "temp": round(c, 1), "temp_text": ui.temp_text(c), "cpu_mhz": microcontroller.cpu.frequency // 1000000,
        "mem_free": gc.mem_free() // 1024, "mem_used": gc.mem_alloc() // 1024,
        "disk_free": st[0] * st[3] // 1024, "disk_total": st[0] * st[2] // 1024,
        "uptime": int(time.monotonic()), "ip": str(r.ipv4_address), "rssi": r.ap_info.rssi if r.connected else None,
        "ssid": str(r.ap_info.ssid) if r.connected else None,
        "app": ui.state.get("app", "menu"), "apps": APPS,
        "scenes": [cls.name for cls in scenes.ALL], "scene": ui.state.get("ls_index", 0),
        "speed": ui.state.get("ls_speed", 1.0), "settings": ui.settings,
        "alt_m": None if alt is None else round(alt, 1), "hpa": None if hpa is None else round(hpa, 1),
        "imperial": ui.imperial(),
    }


def _altlog_csv():
    nvm = microcontroller.nvm
    at = 1024
    if bytes(nvm[at:at + 2]) != b"AL":
        return "minutes_ago,altitude_m,altitude_ft\n"
    count, head, interval = struct.unpack("<HHH", bytes(nvm[at + 2:at + 8]))
    cap = (len(nvm) - at - 8) // 2
    raw = bytes(nvm[at + 8:at + 8 + cap * 2])
    start = (head - count) % cap
    rows = ["minutes_ago,altitude_m,altitude_ft"]
    for i in range(count):
        m = struct.unpack_from("<h", raw, ((start + i) % cap) * 2)[0] / 2
        rows.append("%.1f,%.1f,%.0f" % ((count - 1 - i) * interval / 60, m, m * 3.28084))
    return "\n".join(rows) + "\n"


def start():
    """Start the panel if Wi-Fi is up. Safe to call more than once."""
    global _server
    if _server is not None:
        return True
    import wifi
    if not wifi.radio.connected:
        return False
    import socketpool
    import adafruit_httpserver as hs
    pool = socketpool.SocketPool(wifi.radio)
    server = hs.Server(pool, "/www", debug=False)

    def body(request):
        try:
            return json.loads(request.body or b"{}")
        except ValueError:
            return {}

    def ok(request, data=None):
        return hs.JSONResponse(request, data if data is not None else {"ok": True})

    @server.route("/")
    def index(request):
        return hs.FileResponse(request, "index.html", "/www")

    @server.route("/api/status")
    def status(request):
        return ok(request, _status())

    @server.route("/api/scene", hs.POST)
    def scene(request):
        import scenes
        ui.state["ls_index"] = int(body(request).get("i", 0)) % len(scenes.ALL)
        return ok(request)

    @server.route("/api/speed", hs.POST)
    def speed(request):
        ui.state["ls_speed"] = max(0.1, min(4.0, float(body(request).get("v", 1.0))))
        return ok(request)

    @server.route("/api/settings", hs.POST)
    def settings(request):
        changed = False
        for k, v in body(request).items():
            if k in ALLOWED and v in ALLOWED[k]:
                ui.settings[k] = v
                changed = True
        if changed:
            ui.save_settings()
            ui.apply_settings(ui.state.get("px"))
        return ok(request)

    @server.route("/api/launch", hs.POST)
    def launch(request):
        name = body(request).get("app")
        if name in [a for a, _ in APPS]:
            ui.state["launch"] = name  # the current app sees this and steps aside
        return ok(request)

    @server.route("/api/altlog.csv")
    def altlog(request):
        return hs.Response(request, _altlog_csv(), content_type="text/csv",
                           headers={"Content-Disposition": "attachment; filename=altitude-log.csv"})

    try:
        server.start(str(wifi.radio.ipv4_address), 80)
    except OSError as e:  # port 80 still held by CircuitPython's dashboard until a reset
        print("control panel couldn't start:", e)
        return False
    _server = server
    ui.web_poll = poll
    print("control panel at http://%s/" % wifi.radio.ipv4_address)
    return True


def poll():
    global _last
    now = time.monotonic()
    if now - _last < 0.03:
        return
    _last = now
    try:
        _server.poll()
    except (OSError, RuntimeError, ValueError) as e:
        print("panel:", e)
