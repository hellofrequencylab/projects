# CyberMesh

World-wide emergency mesh: signed, content-addressed objects over any link,
with ESP-NOW on cheap ESP32s as the floor. **Status: design proposal.**
`docs/architecture-v0.md` is the current proposal and
`docs/research-2026-10-03.md` holds its evidence. Neither is a spec. Don't
treat anything in `docs/` as settled, and don't write code on top of it unless
the owner says to.

## Pinned toolchain

Undecided, and nothing here builds yet. Before adding any code, pin the
**exact** versions here: language/runtime, SDK (e.g. ESP-IDF tag), build, test.

- Language / runtime: TBD
- Install: TBD
- Build: TBD
- Test: TBD

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

## Constraints that bite

- An ESP32 running a SoftAP hotspot and ESP-NOW together has to keep both on
  the same Wi-Fi channel.
