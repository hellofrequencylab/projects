# CyberMesh

Community-built backup internet: DIY ESP32 "cyber boxes" that sync signed notes
and files with any box in range, with no ISP, accounts or central server.

**Status: brainstorm.** No code and no chosen stack, so there's nothing to build
or run yet. The first design conversation is recorded in
[`docs/handoff-2026-10-03.md`](docs/handoff-2026-10-03.md). It's a loose
starting point, not a spec.

## Decided

Nothing yet.

## Open questions

- **Repo home.** The handoff says CyberMesh "needs its own repo." It lives here
  for now while it's a brainstorm.
- **First target board.** The handoff suggests the ESP32-S3. `aio-board/` has an
  ESP32-S2. Check that ESP-NOW Long Range works on the S2 in the pinned ESP-IDF
  headers before assuming it.
- **Reticulum on the heart.** Does the ESP32 run Reticulum itself, or only
  ESP-NOW framing with Reticulum on the host? This decides firmware size and
  language.
- **The shared channel.** A hotspot and ESP-NOW on one ESP32 share a channel.
  Which channel, and what happens when local Wi-Fi is crowded on it?
- **Range and throughput numbers** are unverified until a field test.
