# unicorn-os-circuitpython

"Unicorn OS": a CircuitPython app launcher for an Adafruit Feather ESP32-S3 TFT,
driving an 8x8 WS2812B LED panel. Archived from a scratch workspace (2026-10-17);
being ported to C++ in `cyberpunkunicorn-os/` (separate branch/project). Do not
port or edit the other project from here.

## Pinned toolchain

- Runtime: **CircuitPython 10.3.1** for `adafruit_feather_esp32s3_tft` (en_US).
  Evidence: the only firmware in the source workspace was
  `adafruit-circuitpython-adafruit_feather_esp32s3_tft-en_US-10.3.1.uf2`. The code
  itself does not check a version. **Library bundle version is unknown** (not
  recorded); use the 10.x bundle matching the firmware.
- Libraries needed in `CIRCUITPY/lib` (from imports): `adafruit_display_text`,
  `adafruit_ticks`, `adafruit_bmp280` (+ `adafruit_bus_device`/`register` deps as
  the bundle requires), `adafruit_httpserver`. Built-ins used: `displayio`,
  `vectorio`, `keypad`, `neopixel_write`, `gifio`, `jpegio`, `wifi`,
  `socketpool`, `_bleio`, `microcontroller.nvm`.
- Deploy: copy the *contents* of `launcher/` to the root of the CIRCUITPY drive
  (`code.py`, `ui.py`, `scenes.py`, `webui.py`, `apps/`, `www/`, `slides/`), put
  libs in `CIRCUITPY/lib`, and copy `settings.example.toml` to `settings.toml`
  there. The board auto-reloads.
- Build/test: none. No simulator; code only runs on the board. Host tools
  (`launcher/tools/*.py`) need Pillow (+ `gifsicle` for `fit_gifs.py`).

## Hardware (read off the code)

- Board: Feather ESP32-S3 TFT: 240x135 ST7789 (`board.DISPLAY`), BOOT button
  (`board.BUTTON`), onboard NeoPixel (`board.NEOPIXEL` / `NEOPIXEL_POWER`).
- LED panel: 8x8 WS2812B, data on **D5** (override `LS_PIN`), V+ to USB, V- to GND.
  Optional `LS_ZIGZAG=1`, `LS_ROTATE=0/90/180/270`.
- Sensor: BMP280 on STEMMA QT / I2C (`board.I2C()`), address **0x77**.
- Radios: Wi-Fi and BLE are both used, in different apps (not simultaneously).

## Grounding rules

CircuitPython core APIs change between majors. Before changing a call, check it
against the 10.x docs / the installed bundle; do not trust memory. Note
`math.hypot` does not exist in CircuitPython (see `scenes.py`).

## Constraints that bite

- Single button UI: tap = next, double-tap = back, hold = open/act
  (`ui.Button`). Every app must call `btn.poll()`/`btn.wait()` often: the web
  panel is polled from inside it.
- Memory is tight: apps are imported on launch and removed from `sys.modules` on
  exit, with `gc.collect()`. Keep that pattern.
- Settings and Wi-Fi credentials live in **`microcontroller.nvm`**, not files
  (CIRCUITPY is read-only to code while USB-mounted). Layout: `[0]` menu position,
  `[64:576]` settings JSON, `[600:1000]` saved Wi-Fi (plaintext), `[1024:]`
  altimeter ring log. Don't overlap these ranges.
- Power: 64 LEDs at full white is ~3.8 A; brightness steps top out at 0.4. Keep it.
- Light Show leaves the data pin driven low after exit on purpose (floating line
  held the last frame).
- Web UI is on port 80; CircuitPython's own dashboard moves to 8080.
- **Secrets:** never commit `settings.toml` (gitignored). Only
  `settings.example.toml` with placeholders.
- No Weather app exists in the code, though `settings.toml` mentions one.
- Wi-Fi Setup hosts an open-to-guess AP `Unicorn-Setup` with a hardcoded
  password (`AP_PASSWORD` in `apps/wifi_setup.py`, shown on the board's screen).
  It is a device default, not a user credential; change it if you care.
