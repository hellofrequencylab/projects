#!/usr/bin/env python3
"""Mirror the CYD's screen to a browser on the Mac and drive it, over USB serial.

Uses Bruce's own serial CLI (all at the pinned 1.16.1 tag):
  display start  -> streams every TFT draw call as binary packets (tftLogger.cpp)
  nav <button>   -> presses sel/esc/up/down/next/prev (util_commands.cpp)
Packet format: 0xAA <size> <fn> <big-endian uint16 args...> [string] (tftLogger.h).

Opening the port resets the ESP32 (CH340 + macOS), so we wait for Bruce to boot
before sending `display start`.

Only `nav` and `option` are safe to send while an app runs: any other command
makes Bruce call backToMenu() (serialcmds.cpp), which quits the app. So the
remote never polls with other commands.

Bruce also turns screen logging off behind our back (stopWebUi() calls
tft.setLogging(false)), which silently ends the stream. We restart it only when a
button press drew nothing AND Bruce says it is on the Main Menu, where the
backToMenu() side effect is harmless. Restart is stop, pause, start: the old async
task deletes the queue ~100 ms after `stop`, and would delete the new one if
`start` came first.

Run: ./remote.sh   (or .venv/bin/python remote/bruce_remote.py [--port P] [--http 8765])
"""
import argparse
import base64
import glob
import json
import os
import sys
import threading
import time
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import serial

HERE = os.path.dirname(os.path.abspath(__file__))
BAUD = 115200
BOOT_WAIT_S = 14  # boot + WebUI autostart takes ~12 s on this unit
HEADER = 0xAA
VALID_FNS = set(range(0, 22)) | {99}
FILLSCREEN, SCREEN_INFO = 0, 99
NAV_BUTTONS = {"sel", "esc", "up", "down", "next", "prev"}


class Hub:
    """Serial owner: parses the stream, keeps a replay log, fans out to browsers."""

    def __init__(self, port):
        self.port = port
        self.ser = None
        self.cond = threading.Condition()
        self.events = []  # (seq, dict)
        self.seq = 0
        self.screen = []  # packets since the last full-screen clear, for late joiners
        self.info = None  # last SCREEN_INFO packet
        self.status = "starting"
        self.write_lock = threading.Lock()
        self.last_pk = 0.0  # time of the last screen packet
        self.since_nav = ""  # serial text since the last button press

    # -- fan-out -------------------------------------------------------------
    def emit(self, ev):
        with self.cond:
            self.seq += 1
            self.events.append((self.seq, ev))
            if len(self.events) > 2000:
                del self.events[:1000]
            self.cond.notify_all()

    def set_status(self, s):
        self.status = s
        self.emit({"t": "status", "v": s})

    def snapshot(self):
        with self.cond:
            pk = ([self.info] if self.info else []) + list(self.screen)
            return self.seq, pk

    # -- serial --------------------------------------------------------------
    def send(self, line):
        if not self.ser:
            return False
        with self.write_lock:
            self.ser.write((line.strip() + "\n").encode())
        return True

    def nav(self, button):
        t0 = time.time()
        self.since_nav = ""
        ok = self.send("nav " + button)
        if ok:
            threading.Timer(1.2, self.check_stream, args=(t0,)).start()
        return ok

    def check_stream(self, t0):
        if self.last_pk < t0 and "Actual Menu: Main Menu" in self.since_nav:
            self.restart_stream()

    def restart_stream(self):
        self.send("display stop")
        time.sleep(0.5)
        self.send("display start")

    def open(self):
        s = serial.Serial()
        s.port, s.baudrate, s.timeout = self.port, BAUD, 0.05
        s.dtr = s.rts = False
        s.open()
        self.ser = s

    def run(self):
        while True:
            try:
                self.set_status(f"opening {self.port}")
                self.open()
                self.set_status("board rebooting…")
                t = time.time()
                while time.time() - t < BOOT_WAIT_S:
                    self.text(self.ser.read(4096))
                self.send("display start")
                self.set_status("connected")
                self.read_loop()
            except (serial.SerialException, OSError) as e:
                self.set_status(f"disconnected: {e}")
                try:
                    self.ser and self.ser.close()
                except Exception:
                    pass
                self.ser = None
                time.sleep(2)

    def text(self, b):
        if not b:
            return
        t = b.decode("utf-8", "replace")
        self.emit({"t": "text", "v": t})
        self.since_nav = (self.since_nav + t)[-2000:]

    def packet(self, p):
        fn = p[2]
        self.last_pk = time.time()
        with self.cond:
            if fn == SCREEN_INFO:
                self.info = p
            elif fn == FILLSCREEN:
                self.screen = [p]
            else:
                self.screen.append(p)
                if len(self.screen) > 20000:
                    del self.screen[:5000]
        self.emit({"t": "pk", "v": base64.b64encode(p).decode()})

    def read_loop(self):
        buf = bytearray()
        while True:
            chunk = self.ser.read(4096)
            if not chunk:
                continue
            buf += chunk
            text = bytearray()
            i = 0
            while i < len(buf):
                if buf[i] != HEADER:
                    text.append(buf[i])
                    i += 1
                    continue
                if i + 3 > len(buf):
                    break  # need size + fn
                size, fn = buf[i + 1], buf[i + 2]
                if size < 3 or fn not in VALID_FNS:
                    text.append(buf[i])  # not a packet after all
                    i += 1
                    continue
                if i + size > len(buf):
                    break  # wait for the rest
                self.packet(bytes(buf[i : i + size]))
                i += size
            del buf[:i]
            self.text(bytes(text))


def make_handler(hub):
    class H(BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def _send(self, code, body, ctype):
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            if self.path in ("/", "/index.html"):
                with open(os.path.join(HERE, "index.html"), "rb") as f:
                    self._send(200, f.read(), "text/html; charset=utf-8")
            elif self.path == "/events":
                self.events()
            else:
                self._send(404, b"not found", "text/plain")

        def do_POST(self):
            n = int(self.headers.get("Content-Length", 0))
            try:
                body = json.loads(self.rfile.read(n) or b"{}")
            except ValueError:
                body = {}
            if self.path == "/nav" and body.get("button") in NAV_BUTTONS:
                ok = hub.nav(body["button"])
            elif self.path == "/cmd" and isinstance(body.get("line"), str):
                ok = hub.send(body["line"])
            else:
                return self._send(400, b"bad request", "text/plain")
            self._send(200 if ok else 503, b"ok" if ok else b"not connected", "text/plain")

        def events(self):
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            seq, pk = hub.snapshot()
            first = [{"t": "status", "v": hub.status}, {"t": "reset"}]
            first += [{"t": "pk", "v": base64.b64encode(p).decode()} for p in pk]
            try:
                self.wfile.write(b"".join(b"data: " + json.dumps(e).encode() + b"\n\n" for e in first))
                self.wfile.flush()
                while True:
                    with hub.cond:
                        hub.cond.wait_for(lambda: hub.seq > seq, timeout=15)
                        new = [e for s, e in hub.events if s > seq]
                        seq = hub.seq
                    out = b"".join(b"data: " + json.dumps(e).encode() + b"\n\n" for e in new)
                    self.wfile.write(out or b": ping\n\n")
                    self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass

    return H


def find_port():
    ports = glob.glob("/dev/cu.usbserial-*") + glob.glob("/dev/cu.wchusbserial-*")
    return ports[0] if ports else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default=os.environ.get("PORT") or find_port())
    ap.add_argument("--http", type=int, default=8765)
    ap.add_argument("--no-browser", action="store_true")
    a = ap.parse_args()
    if not a.port:
        sys.exit("No CYD serial port found. Use a USB-A to USB-C cable into the CYD's own port.")

    hub = Hub(a.port)
    threading.Thread(target=hub.run, daemon=True).start()
    srv = ThreadingHTTPServer(("127.0.0.1", a.http), make_handler(hub))
    srv.daemon_threads = True
    url = f"http://localhost:{a.http}/"
    print(f"Bruce Remote on {a.port} -> {url}  (Ctrl-C to quit)")
    if not a.no_browser:
        webbrowser.open(url)
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        hub.send("display stop")


if __name__ == "__main__":
    main()
