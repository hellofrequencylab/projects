# CyberUnicorn 5000™ Design System

CyberUnicorn 5000 is an operating system for the ESP32-2432S028 "Cheap Yellow Display" (2.8" 320×240 touchscreen) stacked on an NMTech NM-RF-HAT (CC1101, nRF24, PN532 NFC, IR, 433 MHz). It runs Bruce 1.16.1 underneath with its own branding, patches and apps on top: a glitchy synthwave unicorn boot screen, a rainbow skin on every screen, and an app for each function of the boards (Frequency NFC tool, Light Show, Sensors, LED, Light, Sound, Slideshow, Weather, Hat, GPIO, Remote). A browser device console in the Frequency admin (frequencylocal.com) talks to it over Web Serial / Web Bluetooth; that console uses the **Frequency** design system, not this one.

This system covers the device itself: its screens, its app icons and its brand.

## Sources
- GitHub `hellofrequencylab/projects`, branch `main`, path `cyberpunkunicorn-os/` (read 2026-10-20, commit 03d17daa69d5). Key files: `apps/boot/unicorn_boot.cpp` (boot scene), `assets/make_unicorn.py` (unicorn sprite + palette), `apps/ui/rainbow.cpp` (skin helpers), `apps/*/…Menu.cpp` (each app's previous `drawIcon`), `apps/apps.h` (app registry and order), `CLAUDE.md`, `boot.conf`.
- Copies of the menu sources used are in `cyberpunkunicorn-os/` for reference.
- Handoff "CyberUnicorn Device Console in frequencylocal.com admin" (pasted, 2026-10-20).
- Bruce firmware source was not available; the main-menu layout is approximated.

## Index
- `styles.css` → `tokens/colors.css`, `typography.css`, `spacing.css`, `motion.css`
- `icons/` — **the app icon set**
  - `build_icons.js` — source of truth: every icon as shape calls on a 32×24 grid
  - `generate.js` — regenerates the outputs below
  - `gen_firmware.js` — writes `apps/ui/cyber_icons.h` (run-length, ~4.2 KB flash) and `--preview out.png`
  - `icons_preview.png` — sheet of all icons, frame A (top) and B (bottom)
- `components/icons/` — PixelIcon, UnicornSprite, `iconData.js` (generated), `pixel.js` (firmware colour maths), `unicornData.js`
- `components/rainbow/` — RainbowText, RainbowFrame, RainbowBar
- `components/device/` — DeviceScreen, BootScene, MenuScreen, AppFrame, StatusBar, OptionList, ScreenText
- `ui_kits/device/` — click-through: boot → menu → every app
  - `apps/` — one screen set per app (`Frequency.jsx`, `AppsA.jsx`, `AppsB.jsx`, `shared.jsx`) and one "Apps" card per app
- `guidelines/` — foundation cards (colours, type, spacing, motion, brand, icons)
- `assets/` — `boot_preview.png`, `unicorn_preview.png` (from the repo)
- `SKILL.md`, `thumbnail.html`

## Components
- **PixelIcon** — animated 32×24 app icon (23 ids)
- **UnicornSprite** — the boot unicorn, galloping
- **RainbowText** — per-letter hue text (`cyberRainbowText`)
- **RainbowFrame** — hue-walking outline (`cyberRainbowFrame`)
- **RainbowBar** — rainbow progress bar with cyan outline (boot loader, `cyberRainbowFill`)
- **DeviceScreen** — yellow CYD bezel around a 320×240 screen
- **BootScene** — canvas port of the boot screen plus the blast-off finale: the unicorn trembles during the loader stall, banks left, rockets over the sun into the distance and its rainbow spray floods the screen as the loading transition (10 s). In firmware since 2026-10-20 (`apps/boot/unicorn_boot.cpp`).
- **MenuScreen** — main menu with the rainbow skin
- **AppFrame** / **StatusBar** — `drawMainBorderWithTitle`: rainbow border, status bar, rainbow title at y 28, content from y 46, grey key-hint footer at y 206
- **OptionList** — on-device pick list (`loopOptions` submenu), neon row colours (`cyberItemColor`)
- **ScreenText** — `tft.drawString` in device pixels, GFX sizes 1-4

Intentional additions: all of these are web counterparts of firmware drawing code (rainbow.h, unicorn_boot.cpp, each `drawIcon`); there is no web component library in the source.

## App icons
The brief: make every app icon as good as the unicorn intro. The old icons were thin vector outlines in `priColor` with a few rainbow strokes. The new set uses the intro's own language:

- **Pixel art on the unicorn's palette.** 32×24 cells, 3 device px per cell (96×72 on the CYD), the same scale as the unicorn (`UNI_SCALE 3`). Palette indices 0–9 are identical to `unicorn_sprite.h`; 10–15 add the boot scene's sky, sun, grid and floor colours.
- **Chunky K outline** auto-drawn round solid shapes, exactly how the unicorn is outlined.
- **Animated rainbow** (`R`): hue walks with time at the mane's speed (+12° per 40 ms tick). Each icon picks one direction: `row` (like the mane), `col`, `diag` or `angle` (round the centre, like `cyberRainbowFrame`).
- **Two-frame motion** (`A`/`a`): pixels that swap every 6 ticks, like the gallop. Waves ripple, the heart beats, DIP 3 flips, rain falls, the cursor blinks.
- **Sparkles** (`S`): twinkle white → cyan → pink → off, like the boot sparks.
- **Grid frames** (`Q`/`q`): grid-blue lines that swap with the floor colour, so grids rush like the boot floor.
- **Scenes in miniature**: Light is the boot sun with its stripes; Slideshow frames a tiny boot scene; GPS drops a pin on the synthwave grid; Connect shows two yellow CYDs.

Set (apps.h order, then Bruce menus), with descriptive menu labels: Frequency Check-in, LED Light Show, Sensor Hub, RGB LED, Light Meter, Sound & Tones, Photo Slideshow, Weather Forecast, Hat Module Probe, GPIO Pins, Phone Remote · WiFi Tools, Bluetooth LE, Sub-GHz Radio, RFID Cards, Infrared Remote, 2.4 GHz Radio, GPS & Wardriving, JS Scripts, More Tools, Desk Clock, File Share, Settings.

The labels are design-side; the firmware's `MenuItemInterface("…")` names (and the `Light Show` lookup in LightShowMenu.cpp's remote start) must be renamed together if adopted.

**Frequency Check-in** is the flagship: the Frequency logo traced at 24 px (ring + FR ligature: F stem and top bar running into the R bowl, short F crossbar, R stem and diagonal leg). Synthwave version: 2 px rainbow ring spinning round the angle, 1 px dark gap, night-sky disc, white FR, a striped sun rising behind the legs; sparkles and cross sparks orbit outside. Source logo: `assets/frequency-logo-round.png`.

### Using them in firmware
Done (2026-10-20), see `CHANGES.md`. `node icons/gen_firmware.js` regenerates `apps/ui/cyber_icons.h`; apps call `cyberIcon(getName().c_str(), iconCenterX, iconCenterY, cell, true)` from `apps/ui/cyber_icon.h`; patch 0010 draws Bruce's main-menu pages and option lists with them and redraws animated cells every 40 ms while the menu is idle.

## App screens
Each CyberUnicorn app has its screens in the UI kit and a card in the **Apps** group: the app's real option list, then its main screen(s). Titles, option labels, footers and messages are the strings in each app's `.cpp` (e.g. "TAP A TAG", "NAILED IT", "TURN DIP 3 ON", "SEL reset min/max    ESC exit"). Layout follows the code where it gives coordinates (Frequency `line()` label x 14 / value x 82, Light meter positions, Weather backdrop and 12×12 condition sprites); elsewhere it is composed on the AppFrame grid. Sample readings (temperatures, tag UIDs, Space names) are illustrative.

Screen rules:
- Title: rainbow, size 2, uppercase. Content from y 46. Footer: grey size 1, `KEY action`.
- Hero numbers in rainbow text at size 4; units and bands at size 2 in a hue that means something (temperature, light level).
- Labels grey, values white or a meaningful hue: green ok, cyan/blue links, hot pink attention, grey idle.
- Graphs and bars sit in 1 px rainbow frames; plots are 2-3 px blocks, never anti-aliased lines.
- One instruction per error screen, in plain words ("TURN DIP 3 ON"), with the hardware drawn.

## Proposed beyond the firmware
- Light Show "Fullscreen show": the 8x8 panel at 30 px per LED fills the screen height with a soft glow in the side bars; the scene name and level flash along the bottom for ~2 s after any key.

## Content fundamentals
- Two registers. **Playful** on boot and idle: deadpan technical jokes ("Reticulating rainbows...", "Bribing the CC1101...", "Installing hooves.dll...", "137% (oops)", tagline "All systems fabulous."). **Plain** for anything the user must act on: "Turn DIP 3 ON", "Waking the unicorn…", "Use Chrome or Edge".
- Short imperative sentences, sentence case on web; ALL CAPS only for the boot title and device labels.
- ASCII only on the device (the font has no other glyphs; ™ is drawn as a superscript "TM"). Max 26 characters for boot strings.
- No emoji. Hardware named precisely (CC1101, PN532, DIP 3, GPIO34).
- Second person, no "we" on screen.

## Visual foundations
- **Colour**: black screen (`bgColor`) everywhere; neon accents at full saturation. Default skin is hot pink primary + cyan. Rainbow is the signature: the full hue wheel, starting at hot pink, always moving.
- **The synthwave scene** is the hero background: purple-to-maroon banded sky, blinking stars, a yellow-to-pink sun with horizontal stripe cuts near its base, a magenta-ray / cyan-line grid rushing toward the viewer. Use it for splash and hero moments only, never behind text-heavy screens.
- **Type**: the Adafruit GFX 6×8 bitmap font at sizes 1–3. On the web: Press Start 2P (display) and VT323 (body/console), both stand-ins.
- **Pixels**: integer scaling only, `image-rendering: pixelated`, no anti-aliasing. Everything snaps to the 3 px cell.
- **Motion**: stepped, never eased. 25 fps ticks, two-frame swaps, hue walks. The one "glitch" effect: the title's cyan/magenta chromatic split jitters and a coloured slice flashes across it 1 in 7 redraws.
- **Borders**: 1 px rainbow frames round panels; 1 px cyan outline on progress bars. Square corners on device; radius 4–5 only on rounded panels.
- **Shadows/blur/transparency**: none. The glitch offset (`--shadow-glitch`) is the only shadow-like effect.
- **States**: selected = magenta fill with black text, or rainbow text. Press = magenta fill. Disabled = lavender.
- **Imagery**: pixel art only; photographs only inside Slideshow.

## Iconography
- App icons: the pixel-art set above. Built from code, no icon font, no SVG.
- Status/UI glyphs on the device are drawn with TFT primitives (triangles for arrows, rects for bars). Unicode symbols and emoji are not used: the font can't render them.
- No Bluetooth rune (trademark): the BLE icon is a "BLE" badge.

## Caveats
- Fonts are Google Fonts substitutes for the device bitmap font.
- Main-menu geometry approximates Bruce 1.16.1; confirm `iconCenterX/Y` and the icon area size against `.bruce/` before shipping (`cyberIconCell` assumes ~96×72 fits).
- Icons are untested on the panel: RGB565 rounding, ILI9341 colour and SPI redraw speed should be checked on hardware.
