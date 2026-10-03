# CyberMesh

Community mesh of ESP32 "cyber boxes" that sync signed, content-addressed blobs
over ESP-NOW. **Status: brainstorm.** `docs/handoff-2026-10-03.md` is a
loose first design conversation, not a spec. Don't treat its "decisions" as
settled, and don't write code on top of them unless the owner says to.

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
- The range, throughput, cost and prior-art claims in the handoff are from
  memory. Verify them before repeating them as fact.

## Constraints that bite

- An ESP32 running a SoftAP hotspot and ESP-NOW together has to keep both on
  the same Wi-Fi channel.
