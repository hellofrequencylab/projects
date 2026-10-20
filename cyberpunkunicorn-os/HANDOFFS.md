# Handoffs and merge order

The single source of truth for which session's work is in the firmware, in the
order it landed. **Only the coordinating session merges, builds for the board and
flashes.** Feature sessions commit to their own branch and stop.

The flashable firmware is always the tip of `claude/cyberpunkunicorn-os`, never a
feature branch.

## Merged into `claude/cyberpunkunicorn-os` (in this order)

| # | Branch | Commits | What | Board-verified |
|---|---|---|---|---|
| 0 | (base) | `c796373`, `0d22213` | Project + repo routing: patches 0001–0009, boot scene, rainbow skin, de-brand, Hat, Light Show | Hat (DIP 3 on / all off), boot, `info` |
| 1 | `claude/cu-frequency-3437af` | `a091e08` | **Frequency**: venue tag writer/reader, member-card scan against frequency-web's device API (`apps/frequency/BACKEND.md`) | no |
| 2 | `claude/cu-remote-89a3f9` | `348fe1d` | **Remote**: phone protocol v1 over USB serial + BLE (`apps/remote/PROTOCOL.md`), patch 0014 | no |
| 3 | `claude/cu-sensors-e1baee` | `27dcbd6` | **Sensors**: I2C sensors, ESP-NOW hub, altimeter, CSV log (`apps/sensors/PROTOCOL.md`) | no |
| 4 | `claude/cu-onboard-apps-a451cf` | `60f7c78` `679a5af` `a9eefcb` `bd5f29b` | **LED, Light, Sound, GPIO** (+22,976 B with #5) | boots only |
| 5 | (coordinator) | `dc7b609` | Remote commands wired to LED/Sound/Light/Sensors/Frequency (`apps/remote/app_handlers.cpp`) | no |
| 6 | `claude/cu-onboard-apps-a451cf` | `c903a1d` | Fix: `pins` crash + garbage `light` (Bruce's broken `serialDevice->printf`) | no |
| 7 | `claude/cu-launcher-ports-010d8c` | `8386991` `ade98e3` | **Slideshow** (`/slides` on SD/LittleFS) and **Weather** (synthwave card, Wi-Fi) (+24,912 B) | no |

Conflicts were only in `apps/apps.h` (regenerated from the full app list), and in
`CLAUDE.md`/`SCOPE.md` (both sides kept).

## Waiting

Nothing. Every CyberUnicorn branch is merged (11 apps).

## Separate projects (not part of this firmware)

| Project | Branch | State |
|---|---|---|
| `sensor-node/` | `claude/gallant-curie-530312` @ `18f5748` | ESP-NOW node for the Sensors hub. Builds `esp32dev` (939,168 B) and `aio-board-s2` (860,480 B). Not flashed. `include/sensor_packet.h` is a copy of `apps/sensors/sensor_packet.h` from #3; re-copy it if that header changes. Needs its own root-README routing commit. |
| `unicorn-os-circuitpython/` | `claude/unicorn-os-circuitpython-dbe132` @ `8115e40` | Archive of the old CircuitPython launcher, secrets scrubbed. |

## Board state

The board currently runs a build of `bd5f29b` that the onboard session flashed
directly (it has the `pins`/`light` bugs fixed in #6). It gets replaced by the
next coordinator flash of this branch.

## Board retest checklist (after flashing this branch)

From the onboard handoff. Serial at 115200; wait ~30 s after boot.
- `pins`: 3 lines, no reboot. `light`: sensible %, raw, mV; covered → lower (else flip `CYD_LDR_BRIGHT_LOW`).
- `led 255 0 0` / `0 255 0` / `0 0 255` / `led off` → red, green, blue, off (inverted → `CYD_LED_ACTIVE_LOW=false`).
- `tone 440 500`, `tone jingle`, `tone sweep` (needs a speaker on the connector).
- GPIO app: refuses 22/27 with DIP 3 on; toggle/PWM on 27 with DIP 1–5 off; backlight steady.
- `hat`, `frequency …`, `sensors`, `rpc {"id":1,"cmd":"system.status","args":{}}`.
- Known quirk: Light Show on pin 4 leaves the red onboard LED lit after exit.
