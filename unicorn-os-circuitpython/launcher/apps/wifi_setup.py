# Wi-Fi Setup: the board makes its own network, your phone joins it, and a little web
# page lets you pick your home network and type its password. The choice is saved on the
# chip and joined automatically at every startup. Double-tap BOOT to go back.
import time
import displayio
import socketpool
import wifi
import ui

AP_NAME = "Unicorn-Setup"
AP_PASSWORD = "unicorn123"


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace('"', "&quot;"))


def page(networks, message=""):
    opts = "".join(
        '<label class="net"><input type="radio" name="ssid" value="%s"%s> %s <small>%s</small></label>'
        % (esc(n), " checked" if i == 0 else "", esc(n), "&#9733;" * ui.signal_level(r))
        for i, (n, r) in enumerate(networks))
    return """<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Unicorn Wi-Fi</title>
<style>
body{margin:0;font-family:-apple-system,system-ui,sans-serif;background:#1a0f2e;color:#fff4fb}
.card{max-width:420px;margin:24px auto;padding:22px;background:#2e1d4f;border-radius:22px}
h1{margin:0 0 4px;font-size:24px;background:linear-gradient(90deg,#ff8fb1,#ffc48a,#fff08a,#8ef0c5,#7fdbff,#c9a7ff);
-webkit-background-clip:text;color:transparent}
p{color:#b9a5da;margin:0 0 16px}.net{display:block;padding:12px 14px;margin:8px 0;border-radius:14px;background:#1a0f2e}
small{color:#ffd24d;float:right}input[type=text],input[type=password]{width:100%%;box-sizing:border-box;padding:12px;
border-radius:14px;border:0;margin:6px 0 14px;font-size:16px;background:#fff4fb}
button{width:100%%;padding:14px;border:0;border-radius:16px;font-size:17px;font-weight:600;color:#1a0f2e;
background:linear-gradient(90deg,#ff9edb,#c9a7ff)}.msg{color:#8ef0c5;font-weight:600}
</style></head><body><div class="card"><h1>&#129412; Unicorn Wi-Fi</h1>
<p>Pick your network and type its password. The board saves it and joins.</p>
<p class="msg">%s</p><form method="POST" action="/join">%s
<label class="net"><input type="radio" name="ssid" value=""%s> Other network:</label>
<input type="text" name="other" placeholder="network name" autocapitalize="none" autocorrect="off">
<input type="password" name="pw" placeholder="password (leave blank if open)">
<button type="submit">Join &#10024;</button></form></div></body></html>""" % (
        esc(message), opts, "" if networks else " checked")


def done_page(ssid):
    return ("<!doctype html><meta name=viewport content='width=device-width'>"
            "<body style='background:#1a0f2e;color:#fff4fb;font-family:system-ui;text-align:center;padding:40px'>"
            "<h2>&#10024; Joining %s&hellip;</h2><p style='color:#b9a5da'>Check the board's screen. "
            "Once it says joined, switch your phone back to your normal Wi-Fi.</p>" % esc(ssid))


def scan():
    seen = {}
    wifi.radio.enabled = True
    try:
        wifi.radio.stop_scanning_networks()  # in case an earlier scan was left running
    except RuntimeError:
        pass
    try:
        for n in wifi.radio.start_scanning_networks():
            if n.ssid and (n.ssid not in seen or n.rssi > seen[n.ssid]):
                seen[n.ssid] = n.rssi
    finally:
        wifi.radio.stop_scanning_networks()
    return sorted(seen.items(), key=lambda kv: -kv[1])[:12]


def screen(lines):
    g = ui.screen()
    ui.header(g, "WI-FI SETUP", ui.PINK, "2x tap: back")
    for i, (text, color) in enumerate(lines):
        g.append(ui.lbl(text, 10, 30 + i * 16, color))
    return g


def run(btn, px):
    import adafruit_httpserver as hs

    ssid, _ = ui.load_wifi()
    if wifi.radio.connected:
        screen([("Connected to:", ui.DIM), (ui.clip(str(wifi.radio.ap_info.ssid), 30), ui.GOOD),
                ("IP " + str(wifi.radio.ipv4_address), ui.FG), ("", ui.FG),
                ("hold: set up a different one", ui.PINK)])
        while True:
            e = btn.wait(0.5)
            if e == "double":
                return
            if e == "long":
                break

    ui.settings["wifi"] = True
    wifi.radio.enabled = True
    screen([("Looking for networks...", ui.DIM)])
    px.color(ui.ACCENT)
    try:
        networks = scan()
    except (RuntimeError, OSError, ConnectionError) as err:
        networks = []  # the page still lets you type a network name
        print("scan failed:", err)
    try:
        wifi.radio.stop_station()
    except (AttributeError, RuntimeError):
        pass
    message = ""
    while True:
        wifi.radio.start_ap(AP_NAME, AP_PASSWORD)
        ip = str(wifi.radio.ipv4_address_ap)
        pool = socketpool.SocketPool(wifi.radio)
        server = hs.Server(pool, debug=False)
        chosen = []

        @server.route("/")
        def index(request):
            return hs.Response(request, page(networks, message), content_type="text/html")

        @server.route("/join", hs.POST)
        def join(request):
            form = request.form_data
            name = (form.get("ssid") or form.get("other") or "").strip()
            if not name:
                return hs.Response(request, page(networks, "Pick or type a network name"),
                                   content_type="text/html")
            chosen.append((name, form.get("pw") or ""))
            return hs.Response(request, done_page(name), content_type="text/html")

        server.start(ip, 80)
        screen([("1. On your phone, join Wi-Fi:", ui.DIM), ("   " + AP_NAME, ui.PINK),
                ("   password: " + AP_PASSWORD, ui.FG), ("2. Open in your browser:", ui.DIM),
                ("   http://" + ip, ui.ACCENT), (message or "found %d networks nearby" % len(networks),
                                                  ui.WARN if message else ui.DIM)])
        px.color(ui.PINK)
        while not chosen:
            try:
                server.poll()
            except (OSError, RuntimeError):
                pass
            e = btn.wait(0.02)
            if e == "double":
                server.stop()
                wifi.radio.stop_ap()
                ui.connect_wifi()
                return
        time.sleep(1)  # let the phone receive the "joining" page
        server.stop()
        wifi.radio.stop_ap()

        name, pw = chosen[0]
        screen([("Joining", ui.DIM), (ui.clip(name, 30), ui.PINK), ("...", ui.DIM)])
        px.color(ui.ACCENT)
        try:
            wifi.radio.connect(name, pw, timeout=15)
        except (ConnectionError, OSError, RuntimeError, ValueError) as err:
            message = "Couldn't join %s: %s" % (name, ui.clip(str(err) or "check password", 18))
            continue
        if wifi.radio.connected:
            ui.save_wifi(name, pw)
            ui.save_settings()
            screen([("Joined!", ui.GOOD), (ui.clip(name, 30), ui.PINK),
                    ("IP " + str(wifi.radio.ipv4_address), ui.FG), ("", ui.FG),
                    ("Saved: joins at every startup", ui.DIM), ("Switch your phone back now", ui.DIM)])
            px.color(ui.GOOD)
            while btn.wait(1) != "double":
                pass
            return
        message = "Couldn't join %s, try again" % ui.clip(name, 16)
