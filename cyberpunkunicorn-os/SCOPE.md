# CyberUnicorn 5000™ — scope

Status (2026-10-19): phase 1 done (branded build boots), phase 2 **Hat** app on
the board, plus the full rainbow redesign (boot scene, skin, de-brand). Written 2026-10-19 from the Bruce 1.16.1
source and the `nm-rf-hat/` notes. Anything marked *(verify)* comes from common
CYD documentation, not from this board — check it on the hardware before coding
against it.

## Goal

Our own firmware for the CYD (ESP32-2432S028) + NM-RF-HAT, with **one app for
every hardware function of the combined boards**, and built to be the hub for
**external sensors** we make ourselves. First custom app: **Frequency** (RFID
programmer and scanner for a loyalty rewards program).

## How it is built

- Base: **Bruce 1.16.1**, env `CYD-2432S028`, same patch-stack method as
  `nm-rf-hat/` (copied in, not imported — projects don't share code).
- Our apps are plain C++ in `apps/<name>/`; `build.sh` copies them into the Bruce
  tree (like `nm-rf-hat/assets/`). One patch registers them in the main menu
  (`src/core/main_menu.cpp`, the `_menuItems` list). App code never lives in a patch.
- Each app is a `MenuItemInterface` subclass, same as Bruce's own menus
  (`src/core/menu_items/*.h`).
- Branding: boot text/scene (existing patches 0001/0002), a cyberpunkunicorn
  theme, device name in `info`.
- `nm-rf-hat/` stays as the known-good fallback. Full flash backup of the
  current board: `nm-rf-hat/backups/cyd-flash-2026-10-19-bruce-v1.16.1-nm.bin`.

## The constraint that shapes everything

**Pins 22 and 27 are shared by almost everything on the hat.** CC1101 and nRF24
use them as CS/GDO0 on the SPI bus (18/19/23); PN532 and the Grove I2C port use
them as SDA=27 / SCL=22; IR and 433 OOK use them as TX/RX. That is why the DIP
switch allows only one of 1–5 ON at a time. Consequences:

- Only one radio is live at once. Software can't read the DIP switch, but it can
  **probe** which module answers (PN532 at I2C 0x24, CC1101/nRF24 version
  registers over SPI). That becomes the **Hat** app.
- **I2C sensors coexist with the PN532** (I2C is addressed, so they share the
  bus) but not with the SPI radios or IR/433.
- Pin 35 is input-only and free-ish (on CYD P3) *(verify)*.

**Flash:** ~315 KB free in the app partition (3.75 MB) after today's build.
Enough for Frequency plus a few small apps. More needs Bruce modules compiled
out (LoRa, Ethernet, FM are already hidden in the menu but still compiled in).

## Hardware → app map

| Hardware | Pins | App | Status |
|---|---|---|---|
| CC1101 Sub-GHz (DIP 1) | SPI + CS27, GDO0 22 | RF (Bruce) | exists |
| nRF24 2.4 GHz (DIP 2) | SPI + CS27, CE22 | NRF24 (Bruce) | exists |
| PN532 NFC 13.56 MHz (DIP 3) | I2C 27/22 | RFID (Bruce) + **Frequency** | Bruce exists; Frequency built, not yet tested with tags |
| IR TX/RX (DIP 4) | 22 / 27 | IR (Bruce) | exists |
| 433 MHz OOK module (DIP 5) | 22 / 27 | RF (Bruce, simple-module mode) | exists, needs a one-tap module switch |
| DIP switch / which module is live | probe | **Hat** — detect + tell you which switch to flip | built (PN532 verified) |
| Battery (DIP 6) | no sense pin wired in Bruce | **Power** — only if a battery ADC pin exists *(verify)* | new, maybe |
| Wi-Fi | — | WiFi (Bruce) | exists |
| Bluetooth LE | — | BLE (Bruce) | exists |
| ESP-NOW | — | **Sensors** receiver (own callback; coexists with Bruce `core/connect` by being modal) | built, untested |
| Display + touch | — | system | exists |
| RGB LED | 4/16/17 *(verify)*, disabled in Bruce's CYD build | **LED** | new (enable `HAS_RGB_LED` or drive directly) |
| Light sensor (LDR) | GPIO34 *(verify)* | **Light** meter | new |
| Speaker | GPIO26 *(verify)*, not enabled in Bruce's CYD build | **Sound** (tones/alerts) | new |
| microSD | SPI + CS5 | Files (Bruce) | exists; card currently not mounting (fake 2 TB card) |
| BOOT button | GPIO0 | system | exists |
| USB serial | — | CLI (Bruce) | exists |
| Grove/JST ports on hat & CYD | mostly 22/27 *(verify each)* | **Sensors** (wired I2C) | built, untested |
| Free GPIO | 35 (in) *(verify)* | **GPIO** (Bruce has `gpio` serial cmds) | add on-screen app |

## External sensors — the platform piece

Built 2026-10-19 as the **Sensors** app (`apps/sensors/`, see CLAUDE.md "Our
apps"); not yet tested on hardware. Both paths share one list on screen:

1. **Wired (I2C on the Grove port, SDA 27 / SCL 22).** Scans the bus, recognises
   chips by address + ID register, reads them with our own register-level drivers
   (Bruce 1.16.1 doesn't link SensorLib, so nothing was added): BME280/BMP280
   (0x76/77), SHT3x (0x44/45), AHT2x (0x38), BH1750 (0x23/5C), SGP30 (0x58),
   MPU6050/6500/9250 (0x68/69), INA219 (0x40-4F, assumes a 0.1 ohm shunt).
   Other addresses get a "maybe X" name (OLED, ADS1115, SCD4x, ...). Works
   alongside the PN532 (DIP 3); refused while the CC1101/nRF24 is on.
   **Altimeter** view (replaces the CircuitPython launcher's): relative altitude
   from any pressure reading, zeroable.
2. **Wireless sensor nodes over ESP-NOW.** The contract is
   `apps/sensors/sensor_packet.h` (28-byte versioned packet: magic, version, node
   id, type, float value, unit, battery %, sequence, 12-char name), documented in
   `apps/sensors/PROTOCOL.md` with a minimal node sketch. Node firmware will be
   its own project directory (the `aio-board/` ESP32-S2 is a candidate) and
   copies the header unchanged. Optional later: 433 MHz off-the-shelf weather
   sensors via the CC1101.

Logging: CSV every 10 s to SD when mounted, else LittleFS (capped + rotated).
CLI: `sensors`, `sensors scan`, `sensors listen [s]`, `sensors log on|off`.

Next: a node project; drivers for SCD4x (true CO2) and BME680 if wanted;
optional background logging (today it logs only while a Sensors screen is open).

## Frequency (loyalty NFC)

Frequency is the user's loyalty platform, **frequency-web** (Next.js + Supabase,
prod `https://frequencylocal.com`). The device app is its NFC tool, built
2026-10-19 in `apps/frequency/`; the full contract is in
[`apps/frequency/BACKEND.md`](apps/frequency/BACKEND.md).

- Hardware: PN532 over I2C (DIP 3) — **13.56 MHz only** (NTAG213/215/216). No 125 kHz.
- **Program venue tag (works today):** writes a Space's signed check-in URL
  (`/n/<uuid>?s=v1.…`, copied from Space settings → Check-in) or an event door
  code (`/q/<slug>?m=nfc`), reads it back, logs it, optional permanent lock.
  URLs come from the keyboard, `/frequency/tags.txt` (`label|url`, SD or
  LittleFS) or `frequency tag <url>` over serial. Members tap with their own
  phone; frequency-web does the crediting.
- **Verify tag:** shows what any tag points to (venue / event / member card / other).
- **Member scan + issue card (proposed, off by default):** cards carry
  `/c/<token>`; the device POSTs to `/api/v1/devices/scan` and
  `/api/v1/devices/cards` with an `X-Frequency-Device` header. **Those endpoints
  don't exist in frequency-web yet.** No points or ledger on the device; only a
  capped offline retry queue.

## Phases

1. Project skeleton: copy the build, rename/brand, build, flash, confirm boot.
2. **Hat** app (module probe) — every other app depends on knowing what's live.
3. **Frequency**: venue-tag programmer + verifier (built); member scan waits on the backend.
4. Onboard extras: **LED**, **Light**, **Sound** (verify pins first).
5. **Sensors**: I2C wired + ESP-NOW receiver built (untested); next the first sensor-node project.
6. Trim unused Bruce modules as flash runs out.

## Open questions

1. Frequency backend: will frequency-web add the device API (device auth,
   member cards, UID binding)? Until then only venue tags work. Details and
   sub-questions in `apps/frequency/BACKEND.md`.
2. Card type: is the blue fob 125 kHz (unreadable here) or 13.56 MHz?
3. Frequency user: staff device at a counter, or only a tag-programming tool?
4. Which Bruce apps to drop if flash runs short?
5. External sensors: wired, wireless, or both — and what are you measuring?
