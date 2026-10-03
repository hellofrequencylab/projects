# CyberMesh plan

This file is **the plan of record**. The reasoning and evidence behind it live
in [`docs/architecture-v0.md`](docs/architecture-v0.md) and
[`docs/research-2026-10-03.md`](docs/research-2026-10-03.md). To change a
locked decision, edit it here with the date and the reason. Don't let the code
quietly drift away from it.

## Vision

CyberMesh is a world-wide, emergency-capable network that moves signed,
content-addressed objects (notes, files, maps, messages, SOS) between any two
things that can exchange bytes.

- **The floor:** cheap boards radio-to-radio. It works with nothing more than
  a dev board, flash memory and, optionally, a screen.
- **Above the floor:** every other pipe carries the same objects. That
  includes the internet, ham radio, satellite, broadcast, wire and SD cards.
- **When the internet works,** it is just a faster pipe. When it fails, the
  floor keeps working, and new links can be built on top of it.

## Principles

1. Objects are the currency; links are just pipes.
2. Store-and-forward first, real-time second.
3. The floor never moves. Everything faster is optional and negotiated.
4. Legality is a property of each link, and the router enforces it. Ham links
   never carry encrypted objects.
5. Every hash, key and signature carries a suite ID, so crypto can be replaced.
6. The network carries its own bootstrap: the spec, the firmware and the
   build guides are objects in the mesh.
7. Never depend on a company.

## Locked decisions

| # | Decision | Locked | Notes |
|---|---|---|---|
| D1 | **Floor link: ESP-NOW broadcast on 2.4 GHz, channel 1, network-wide.** | 2026-10-03 | Every ESP32 has it, licence-free worldwide. Long Range (LR) mode is a later option once milestone 0 measures it |
| D2 | **Floor frames are ≤ 250 bytes.** | 2026-10-03 | Works with ESP-NOW v1 and v2 receivers, and fits LoRa later |
| D3 | **Floor implementation in portable C** (`core/`, no OS or vendor calls) **plus ESP-IDF glue** (`firmware/`). ESP-IDF pinned in `CLAUDE.md`. | 2026-10-03 | The same core builds on Linux for tests and simulation |
| D4 | **The spec leads the code.** `docs/spec-floor-v0.md` defines the wire format; code that disagrees with it is a bug. | 2026-10-03 | The goal is "two strangers can build compatible nodes from the spec" |
| D5 | **Suite 0x00 (unsigned, SHA-256) is for bring-up only.** Suite 0x01 (Ed25519 + BLAKE3) must land before anyone outside the project runs a node. | 2026-10-03 | A suite-0x00 node trusts anyone in radio range |
| D6 | **Reticulum: bridge to it, don't build on it.** | 2026-10-03 | Its license is no longer OSI open source |
| D7 | **The project lives in this repo until spec v0.1 is done**, then moves to its own repo. | 2026-10-03 | |

## Still open (decide by the milestone shown)

- **Host-side language for Pi and server boxes** (Rust vs C), by milestone 6.
- **Second radio link** (sub-GHz LoRa vs 2.4 GHz SX1280/LR2021), by milestone 5.
- **Whether the public commons is on by default or circles only,** by
  milestone 4. This sets the abuse and legal exposure.

## Roadmap

Every milestone has a "done when" that a stranger could check.

| # | Milestone | Done when | Status |
|---|---|---|---|
| 1 | **Baseline network** (floor v0) | A note typed on a phone joined to box A shows up on a phone joined to box B, with no internet. Box C, out of A's range, gets it through B | **in progress**: core, simulator and firmware written; hardware test pending |
| 0 | **Bench truth** (runs alongside 1) | A measured table of ESP-NOW throughput and range (1 Mbps, 11g, LR) and SoftAP coexistence, on our boards | not started |
| 2 | **Signed objects** (suite 0x01) | Nodes reject a note whose signature fails; identity is a keypair | — |
| 3 | **Persistent store + real reconciliation** | Notes survive reboot; two boxes with 1,000 notes each sync with Negentropy-style range fingerprints | — |
| 4 | **Spaces** | A circle's content crosses a relay that can't read it | — |
| 5 | **Swarm transfer + second link** | A 1 MB file reaches 5 boxes at least 3x faster than sequential unicast; LoRa link added | — |
| 6 | **Home box + internet link** | Two islands in different cities sync over the internet and keep working when it's unplugged | — |
| 7 | **Ham link (signed cleartext) + SD spool** | A note crosses a gap carried only by an SD card | — |
| 8 | **Onboarding** | Someone outside the project flashes a box from a browser and a guide alone | — |

Milestone 1 comes before 0 because a working baseline is the tool the
measurements will need.

## How the baseline network is built (milestone 1)

```
 phone ──Wi-Fi──▶ [box A] ◀─ESP-NOW─▶ [box B] ◀─ESP-NOW─▶ [box C] ◀──Wi-Fi── phone
                 SoftAP + portal       relay                SoftAP + portal
```

Every box runs the same firmware:

- An **open SoftAP** named `CyberMesh-XXXX`, with a **captive portal**, so
  any phone can read and post notes.
- **ESP-NOW broadcast on channel 1**, using the same channel as the SoftAP.
- **The floor v0 sync loop:**
  1. Each box sends a beacon every ~5 s with its note count and a fingerprint
     of its notes.
  2. If a neighbor's fingerprint differs from its own, a box asks for that
     neighbor's inventory.
  3. It then requests the notes it's missing.
  4. Once it has those notes, its own beacon changes, so its neighbors fetch
     them next.
  5. That is how notes travel multiple hops. No routing table is needed.

The first two boxes are the CYD (ESP32) from `nm-rf-hat/` and the AIO Board's
ESP32-S2 from `aio-board/`. Any third ESP32 can be box C.
