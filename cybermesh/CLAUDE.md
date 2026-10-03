# CyberMesh

World-wide emergency mesh: signed, content-addressed objects over any link,
with ESP-NOW on cheap ESP32s as the floor.

**`PLAN.md` is the plan of record.** Its locked decisions (D1–D7) hold until
the owner changes them there. **`docs/spec-floor-v0.md` is normative for
`core/`:** if the code and the spec disagree, fix the code, or change the spec
first and on purpose. `docs/architecture-v0.md` is background reasoning, not
a commitment.

## Pinned toolchain

- **ESP-IDF v6.0.3** (`firmware/build.sh` refuses any other version). Chips
  built in CI: esp32 (CYD), esp32s2 (AIO Board).
- **Install:**

      git clone -b v6.0.3 --recursive https://github.com/espressif/esp-idf
      ./install.sh esp32,esp32s2

  If `dl.espressif.com` is blocked (e.g. in a sandbox), the toolchains still
  come from GitHub. Finish the Python env with
  `tools/idf_tools.py install-python-env --no-constraints`, and export
  `IDF_PYTHON_CHECK_CONSTRAINTS=no` before sourcing `export.sh`.
- **Core:** C99 plus any host `cc`. Run `make -C core test` (ASan/UBSan, with
  `-Werror`).
- **Firmware:** `firmware/build.sh <target> [idf.py args]`. Output goes to
  `firmware/build/<target>/`.
- **Test:** the host tests and simulator in `core/test/`. A clean firmware
  build is the minimum bar, and it is not proof the firmware works on a
  board.

## Grounding rules

- ESP-NOW and Wi-Fi LR behavior differs across ESP32 variants and ESP-IDF
  releases. Once an IDF version is pinned, grep its `esp_now.h` / `esp_wifi.h`
  before using an API or claiming a chip supports a mode.
- Hardware facts come from the owning project and are linked, never copied:
  `aio-board/CLAUDE.md`, `nm-rf-hat/CLAUDE.md`, `flipper/CLAUDE.md`. Per the
  root rules, a CyberMesh change never edits those projects in the same commit.
- The research notes mark each number as sourced (**S**) or estimated (**E**).
  Don't promote an **E** to a fact without a measurement or a primary source.
  No number has been measured on our own boards yet.
- Radio defaults must be legal per region. US 915 MHz LoRa bandwidth rules
  are in flux as of 2026-09. Ham-band links must never carry encrypted
  payloads.

## Code rules

- `core/` is portable: no ESP-IDF, FreeRTOS or libc allocation. Platform
  access goes through `cm_platform` callbacks. Anything new in the core gets a
  host test or a simulator case in `core/test/`.
- Every wire-format change is a spec change first, in `docs/spec-floor-v0.md`.
- The core is not thread-safe. Firmware calls it only under the box mutex
  (`cm_box_lock`).

## Constraints that bite

- An ESP32 running a SoftAP hotspot and ESP-NOW together has to keep both on
  the same Wi-Fi channel (`CONFIG_CM_CHANNEL`, default 1 per D1).
- ESP-IDF v6 has no bundled cJSON. The portal writes its JSON by hand.
- CYD: flash at `-b 230400`. Faster serial corrupts (see `nm-rf-hat/CLAUDE.md`).
- AIO Board: the S2 has native USB only, so the console is USB CDC
  (`sdkconfig.defaults.esp32s2`). The front switch must select the ESP32.
