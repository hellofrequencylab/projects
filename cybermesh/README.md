# CyberMesh

A world-wide, emergency-capable mesh. Signed, content-addressed objects move
over *any* link: cheap ESP32 boards radio-to-radio at the floor, and the
internet, ham radio, satellite, broadcast, wire or an SD card above it.

- [`PLAN.md`](PLAN.md): the plan of record (vision, locked decisions, roadmap)
- [`docs/spec-floor-v0.md`](docs/spec-floor-v0.md): the wire protocol the
  code implements
- [`docs/architecture-v0.md`](docs/architecture-v0.md) and
  [`docs/research-2026-10-03.md`](docs/research-2026-10-03.md): the reasoning
  and evidence behind the plan

**Status: milestone 1, the baseline network.** The core, the simulator and
the box firmware are written and build for ESP32 and ESP32-S2. They have not
been run on hardware yet.

## Layout

| Path | What | Build / run |
|---|---|---|
| `core/` | Portable C99 floor protocol: store, frames, sync | `make -C core test` (host tests plus a simulated mesh) |
| `firmware/` | ESP-IDF app: hotspot, captive portal, ESP-NOW link | `firmware/build.sh esp32` or `firmware/build.sh esp32s2` |

## Set up a baseline network

You need **two or more ESP32 boards**, all running the same firmware. The CYD
(`nm-rf-hat/`, ESP32) and the AIO Board (`aio-board/`, ESP32-S2) both work.

1. **Install ESP-IDF v6.0.3** (see `CLAUDE.md`) and source its `export.sh`.
2. **Flash each box.**
   - CYD: use a USB-A to USB-C cable into the CYD's port, and keep the baud
     rate at 230400:

         firmware/build.sh esp32 -p /dev/cu.usbserial-XXXX -b 230400 flash monitor

   - AIO Board:
     1. Attach the external antenna; the S2 module is the `-U` variant.
     2. Put the front switch in the middle position.
     3. Hold BT while plugging in USB-C, then release it.
     4. Run:

            firmware/build.sh esp32s2 -p /dev/cu.usbmodemXXXX flash

     5. Press RT to boot. Its log comes over the same USB port.
3. **Power the boxes** within a few metres of each other to start with. Each
   one logs `node xxxxxxxx` and raises a hotspot named `CyberMesh-XXXX`.
4. **Join box A's hotspot from a phone.** The portal should open by itself.
   If it doesn't, browse to `http://192.168.4.1`. Post a note.
5. **Join box B's hotspot from a second phone.** The note appears within
   about a second. The header shows how many other boxes this one has heard.
6. **Test a relay:** move box C out of A's range but within B's. Notes still
   reach C through B.

**Milestone 1 is done** when steps 4–6 work and the distances are written in
`PLAN.md`.

Warning: floor v0 is unsigned. Any radio in range can inject notes. Use it
only on your own boxes until milestone 2 adds signatures.
