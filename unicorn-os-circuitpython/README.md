# unicorn-os-circuitpython

Unicorn OS: a pastel app launcher for the Adafruit Feather ESP32-S3 TFT
(CircuitPython 10.3.1), with an 8x8 WS2812B LED panel on D5. Built 2026-10-17 in
a Claude scratch workspace and preserved here unchanged. See [`CLAUDE.md`](CLAUDE.md)
for hardware, deployment and constraints.

## Status

Light Show has been ported to C++ in `cyberpunkunicorn-os/apps/lightshow/`
(separate project). The other apps are being ported. This directory is the
reference for the original behaviour.

## Controls

One button (BOOT): tap = next, double-tap = back/up, hold = open.

## Apps

| App | File | What it does |
|---|---|---|
| Menu | `launcher/code.py` | App list; remembers last selection in nvm; joins saved Wi-Fi and starts the web UI |
| Light Show | `apps/lightshow.py`, `scenes.py` | 11 scenes (Singing Bowl ... Synthwave) on the 8x8 panel with on-screen mirror; hold = brightness |
| Slideshow | `apps/slideshow.py` | BMP/JPEG/GIF from `/slides`; hold shows it on the LED panel |
| Altimeter | `apps/altimeter.py` | BMP280 height, graph, stats; ring log in nvm |
| BT Scanner | `apps/ble_scanner.py` | BLE devices grouped by maker, radar plot |
| Wi-Fi Radar | `apps/wifi_radar.py` | Nearby networks, signal bars, channel congestion |
| Wi-Fi Setup | `apps/wifi_setup.py` | Board hosts an AP + page to pick a network; saved in nvm |
| System | `apps/system.py` | Temp, CPU, memory, storage, radios, settings |
| Web UI | `webui.py`, `www/index.html` | Control panel at `http://<board ip>/`: switch app/scene, speed, settings, altimeter CSV |

**Weather:** the original brief listed a Weather app, but the source contains no
such code. Only a stale `WEATHER_*` comment block in `settings.toml` remains
(kept in `settings.example.toml`). It was probably unfinished or lost.

## Layout

- `launcher/` the CIRCUITPY root contents (`settings.example.toml` to be copied as `settings.toml`)
- `launcher/tools/` host-side Pillow scripts for preparing slides
- `extras/` `bin2uf2.py` (ESP32-S3 .bin to TinyUF2 .uf2) and `hello_tft.ino` (Arduino test sketch)

## Deliberately not committed

Left in the scratch workspace
(`~/Library/Application Support/Claude/scratch-workspaces/098b6011-.../scratch-2026-10-17-57c451/`):

| Path | Size | Why |
|---|---|---|
| `launcher/settings.toml` | 675 B | Credentials file by design (it held placeholders only); example committed instead |
| `launcher/slides/*` (10 files: `00_joost.gif` ... `09_joost.gif`) | ~0.8 MB | Third-party media |
| `launcher/originals/joost.gif`, `joost_reel.gif` | 402 KB, 527 KB | Media |
| `launcher/originals/reel/*` (9 .gif/.webp) | ~0.4 MB | Media |
| `launcher/originals/drive/*` (9 files) | ~24 MB (largest 7.6 MB) | Large third-party media |
| `circuitpython/...10.3.1.uf2` | 3.7 MB | Download from circuitpython.org |
| `hello_tft.uf2`, `build/*.bin/.elf/.map` | 805 KB + build | Build outputs |
| `.DS_Store`, `__pycache__` | n/a | Junk |

`launcher/originals/drive_order.txt` is kept (the order the drive files were used in).
