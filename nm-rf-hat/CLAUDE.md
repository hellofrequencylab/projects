# NM-RF-HAT + CYD — custom Bruce build

Bruce firmware for an ESP32-2432S028 "Cheap Yellow Display" sitting on an
NMTech NM-RF-HAT, with our own patches on top. Hardware facts below were read
off the physical board or the chip over USB on 2026-09-30, unless marked as
from the seller's docs.

## Pinned toolchain

- Bruce: tag **1.16.1**, env **`CYD-2432S028`** (cloned to `.bruce/`, gitignored)
- PlatformIO **6.2.0**, esptool **5.4.0** — in `.venv/`, created by `build.sh`
- Build: `./build.sh` → `build/app.bin`, `build/full.bin`
- Flash: `./flash.sh` (app only) or `./flash.sh --full`
- Test: none automated. A clean build is the minimum bar; confirm on the device.

## How changes are made

Never edit `.bruce/` by hand and leave it — `build.sh` resets it to pristine
upstream on every run. To change Bruce: edit in `.bruce/`, then
`git -C .bruce diff > patches/NNNN-name.patch` (one concern per patch) and reset.
User-tunable values go in `boot.conf` and reach C++ as `-D` macros through
`PLATFORMIO_BUILD_FLAGS`.
Files in `assets/` (e.g. `cat_sprite.h`) are copied by `build.sh` into
`.bruce/src/nm_assets/` after the patches apply; patches `#include "nm_assets/…"`.

## Patches (applied in order by build.sh)

- `0001` boot text → macros (`BOOT_TITLE`/`BOOT_TAGLINE`)
- `0002` cat boot scene (`BOOT_CAT_SCENE`/`BOOT_TRADEMARK`, sprite from `assets/`)
- `0003` radio module **defaults** for this hat: `rfModule=CC1101_SPI`, `rfidModule=PN532_I2C`
- `0004` autostart + menu trim: `startupApp` from `STARTUP_APP`, `disabledMenus`
  from `TRIM_UNUSED_MENUS` (hides FM/LoRa/Ethernet); also adds a serial setter for `startupApp`

**Persisted-config gotcha:** compile-time defaults (0003, 0004) only apply when the
key is absent from the on-flash config (`bruceConf.json` / `brucePins.conf` in
LittleFS at 0x3d0000). Once Bruce has saved a config, that value wins. App-only
flashing keeps that config, so to change a default on an already-used unit, set it
live over serial — `settings rfModule 1`, `settings rfidModule 1`,
`settings startupApp WebUI`, `settings disabledMenus FM` (repeat per menu) — or
`factory_reset` (which also wipes color/rotation). This unit was set live on 2026-09-30.

## Grounding rules

Bruce moves fast. Before touching any Bruce API or config key, grep `.bruce/`
at the pinned tag — not memory, not `main`. Board pins live in
`.bruce/boards/CYD-2432S028/CYD-2432S028.ini`.

## What is actually on it (verified)

| Thing | Value | Source |
|---|---|---|
| MCU | ESP32-D0WD-V3 rev 3.1, dual core, 240 MHz, 4 MB flash | `esptool flash_id` |
| USB-serial | WCH CH340 (VID 0x1A86), macOS built-in driver | ioreg |
| Display | 2.8" 320×240, marked **TPM408-2.8**; Bruce's ILI9341 build renders correctly | on the board / flashed |
| Hat | silkscreen **NM_RF_hat V1.0**, NMTech | on the board |
| Radio SPI | CC1101 & nRF24 share SCK18/MISO19/MOSI23, CS **27**, GDO0/CE **22** | Bruce config dump |

## Constraints that bite

1. **USB-C to USB-C cables do not power this CYD.** It lacks the CC pull-downs,
   so a Mac never turns VBUS on — board is dark and absent from the USB bus.
   Use USB-A → USB-C through an adapter. Plug into the **CYD's** port, not the hat's.
2. **Serial above 230400 baud corrupts** (921600 and 460800 both failed). `flash.sh` uses 230400.
3. **Opening the serial port reboots the board** (auto-reset circuit). Serial
   CLI commands must wait for boot to finish; it has also gone silent at times,
   so the on-device menu is the reliable way to change settings.
4. **DIP switch: only one of 1–5 ON at a time** (1 CC1101, 2 nRF24, 3 PN532,
   4 IR, 5 433 OOK; 6 = battery). Radios share SPI/pins 22/27.
5. **Settings live in LittleFS at 0x3d0000** (layout `custom_4Mb_full.csv`:
   app 0x10000–0x3cffff). App-only flashing preserves them; a full erase does not.
6. **Bruce's `patch.py` weakens a symbol in the shared PlatformIO cache**
   (`~/.platformio/packages/framework-arduinoespressif32-libs/esp32/lib/libnet80211.a`)
   and writes a `lib/.patched` marker even if the patch failed. Symptom:
   `cannot find -lnet80211` or `multiple definition of ieee80211_raw_frame_sanity_check`.
   Fix: if `libnet80211.a.old` exists, rename it back to `libnet80211.a`; delete
   `lib/.patched`; rebuild with `.venv/bin` on PATH (`build.sh` does this).
7. Boot tagline is drawn in the touch footer bar; ~26 chars fit at 320 px.

## Recovery

Factory LVGL demo firmware backup (full 4 MB, SHA-256 `28cc552e…acb5f`) is at
`~/Downloads/cyd_factory_backup_2026-09-30.bin` — not in the repo. Restore with
`.venv/bin/esptool --chip esp32 --port <port> --baud 230400 write-flash 0x0 <file>`.

## Transmit

CC1101/nRF24/433 OOK can transmit. Receive freely; keep TX to hardware you own
and within your local band rules.
