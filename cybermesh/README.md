# CyberMesh

A world-wide, emergency-capable mesh. Signed, content-addressed objects move
over *any* link: $4 ESP32 radio to radio at the floor, and the internet, ham
radio, satellite, broadcast, wire or an SD card in a pocket above it. When
the internet works, it is just a faster pipe.

**Status: design proposal.** No code and no chosen stack, so there's nothing
to build or run yet.

- [`docs/architecture-v0.md`](docs/architecture-v0.md): the proposed
  layers, the minimal "floor" protocol, the tech stack and the roadmap
- [`docs/research-2026-10-03.md`](docs/research-2026-10-03.md): the evidence,
  with sources and confidence marks
- [`docs/handoff-2026-10-03.md`](docs/handoff-2026-10-03.md): the first
  brainstorm

## Decided

Nothing yet. Everything in `docs/` is a proposal.

## Open questions

- **The floor link.** Should the floor be ESP-NOW on 2.4 GHz (every ESP32
  has it), or should every node be required to carry LoRa?
- **Reference core language.** Rust `no_std`, which is memory-safe and
  compiles to WASM, or C, which has a simpler toolchain?
- **The public commons.** On by default, or circles only? This sets the
  abuse and legal exposure.
- **Reticulum.** The handoff planned to build on it, but its new license
  has a field-of-use clause that isn't OSI open source. The proposal is to
  borrow its ideas and bridge to it instead.
- **Repo home.** The handoff says CyberMesh needs its own repo. It lives here
  while it's a design.
- **Bench numbers.** Most throughput and range figures haven't been measured
  yet. Milestone 0 in the architecture doc is a bench test on real boards.
