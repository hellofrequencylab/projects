# CyberMesh architecture, v0 (proposal)

Status: **proposal for discussion**, 2026-10-03. Nothing here is decided. The
evidence and sources are in [`research-2026-10-03.md`](research-2026-10-03.md).
Numbers marked **E** are estimates. Everything else comes from those notes.
None of it has been measured on our own hardware yet.

## The idea in one paragraph

CyberMesh is not a radio network. It is a **way of moving signed,
content-addressed objects between any two things that can exchange bytes**. A
$4 ESP32 talking ESP-NOW to its neighbor is one way. A fiber line, a ham radio,
a satellite modem, a TV broadcast, or an SD card in someone's pocket are
others. The core protocol is small enough to run on a dev board with a screen
and some flash. When the internet works, it is just a very fast pipe. When it
doesn't, the board-to-board floor still works, and everything else rebuilds on
top of it.

## Design principles

1. **Objects are the currency, links are just pipes.** Every link type moves
   the same signed, content-addressed objects. Nothing above the link layer
   knows or cares which pipe was used.
2. **Store-and-forward first, real-time second.** Links will range from
   ~5 bps (HF) to Gbps (fiber), a spread of about 10⁸, and from milliseconds
   to days (sneakernet). Interactive sessions are an optimization, not an
   assumption.
3. **A floor that never moves.** A tiny mandatory core works on the cheapest
   board. Everything faster is optional, negotiated and pluggable.
4. **Legality is a link property, enforced by the router.** Each link declares
   what it may legally carry: whether encryption is allowed, whether
   commercial traffic is allowed, and its duty-cycle and power limits. The
   router never puts an encrypted object on a ham link. Users never have to
   know the rules.
5. **Agility everywhere.** Every hash, key and signature carries a suite ID,
   so the protocol can move to new crypto (post-quantum by the 2030s) without
   a flag day.
6. **The network carries its own bootstrap.** The spec, the firmware, the
   build guides and the flasher are themselves objects in the mesh. Any
   surviving node can teach someone to build the next one.
7. **Never depend on a company.** Starlink, carriers and satellite startups
   are welcome pipes, never the root of anything. Swarm and Othernet both died
   recently, and Starlink has been geofenced and whitelisted.

## Layers

```
 ┌──────────────────────────────────────────────────────────────────┐
 │ Apps: notes · boards · maps · messages · SOS · files · plugins   │
 ├──────────────────────────────────────────────────────────────────┤
 │ Spaces: commons (public, signed) · circles (encrypted) · direct │
 ├──────────────────────────────────────────────────────────────────┤
 │ Sync: range-based set reconciliation over "interests"           │
 ├──────────────────────────────────────────────────────────────────┤
 │ Objects: signed, content-addressed, chunked (BLAKE3 tree)       │
 ├──────────────────────────────────────────────────────────────────┤
 │ Swarm transfer: fountain-coded chunks, broadcast, multi-source  │
 ├──────────────────────────────────────────────────────────────────┤
 │ Router: priorities, airtime budgets, legality, copy budgets     │
 ├──────────────────────────────────────────────────────────────────┤
 │ Links: ESP-NOW · LoRa · FLRC · HaLow · BLE · Wi-Fi/IP · ham     │
 │        satellite · broadcast · serial · wire · SD/USB spool     │
 └──────────────────────────────────────────────────────────────────┘
```

### Links

Each link registers a **link descriptor**:

- `mtu`
- `bitrate`
- `duty_cycle`
- `latency_class` (ms / s / hours / days)
- `cost` (free / metered)
- `broadcast` (yes / no)
- `may_encrypt`
- `may_commercial`
- `energy_per_byte`

The router plans with these and never with radio specifics. Adding a new pipe
means writing one adapter. Examples:

- HaLow in 2027
- Bluetooth HDT
- a 2030s software-defined radio waveform
- a library's ATSC 3.0 datacast

### Objects

Every piece of content is an **object**:

```
object = {
  v:        version
  suite:    crypto suite id (1 B)      # e.g. 0x01 = Ed25519 + BLAKE3-256
  type:     note | file | board-post | map-tile | key-event | tombstone | …
  space:    commons | circle-id | direct-recipient
  author:   public key (or 8 B short ref once the key is known)
  time:     author-claimed timestamp
  refs:     [object ids]               # replies, edits, parents
  body:     inline bytes  |  root hash of a chunk tree
  sig:      signature over all the above
}
id = suite-tagged BLAKE3 hash of the canonical encoding
```

- **Small objects travel in a single frame.** A text note is about 110 B of
  overhead plus the text **E**, which fits one ESP-NOW v2 frame (1470 B)
  easily. It also fits one LoRa frame (~237 B) for short texts.
- **Large objects are chunked under a BLAKE3/Bao tree.** Any chunk from any
  source, over any link, can be verified on arrival. Transfers resume across
  links: start on ESP-NOW, finish from an SD card.
- **There are no per-author hash chains.** Secure Scuttlebutt's logs forked
  and grew forever. CyberMesh syncs *sets* of independent objects instead.
- **Encoding:**
  - On air: a tight binary format with a SCHC-style context byte that implies
    the common fields.
  - On disk and over the internet: deterministic CBOR, so the format can grow.

### Sync

Nodes don't ask "send me everything." They reconcile **interests**: (space ×
type × time range × optionally region).

- **Method:** Negentropy-style range-based set reconciliation. Each node
  sends a fingerprint of a range, and they recursively split the ranges that
  differ.
- **Cost:** two nodes that are nearly in sync agree in a few hundred bytes.
  Two strangers find their differences in a few round trips, with bounded RAM
  per round.
- **Beacons:** the periodic beacon is just a small fingerprint of the node's
  commons index. That replaces the NodeInfo, telemetry and position floods
  that choke Meshtastic meshes.

### Swarm transfer (the speed idea worth inventing)

The biggest speedup on radio doesn't come from a faster modulation. Physics
and regulators cap that. It comes from **sending each byte once to everyone
who wants it**. This combines three proven ideas that no current mesh uses
together:

1. **Broadcast plus content addressing.** Radio is inherently broadcast. If
   five boxes in range all want the same map, one transmission serves all
   five. Content IDs make "who wants what" known through sync.
2. **Fountain coding over chunks.** The sender emits coded symbols, not
   chunks. Any receiver that collects about K symbols, from any mix of
   senders and links, rebuilds the chunk. There are no per-receiver
   retransmits or acknowledgements, and loss on one link doesn't stall the
   others.
3. **Free multi-link bonding.** A box with ESP-NOW, LoRa and an SD card can
   pull symbols from all three at once. The receiver doesn't care where a
   symbol came from, because each one verifies against the tree.

**Worked example (E).**

- Setup: one 1 MB map tile set sent to 10 boxes in range, over ESP-NOW at
  the measured default ~214 kbps.
- Unicast to each box costs about 10 × 40 s = 400 s.
- Coded broadcast costs about 42 s, plus a few percent of overhead, for all
  ten.
- **That is roughly a 10x gain at today's bitrates.** It grows with density,
  which is exactly when a mesh needs it.

**Licensing caveat on the codes:**

- RaptorQ is the best-known code but is covered by Qualcomm patents.
- The candidates are LT/online codes, or a BSD-licensed implementation such
  as Wirehair (license to verify **E**).
- Network coding (RLNC) lets relays recode, but its patent status needs
  checking.

### Router

- **Priority classes, each with an airtime budget:**
  1. SOS and safety
  2. Direct messages
  3. Commons text
  4. Indexes
  5. Bulk files

  Announces and beacons are capped at about 2% of airtime, as Reticulum does.
- **Roles:**
  - *Leaf* nodes, such as a pocket beacon on a battery or a phone portal,
    don't relay.
  - *Relay* nodes, such as rooftops, do.
  - Both Meshtastic and MeshCore found that this split is the most effective
    way to scale.
- **Routing within a local island:** managed flooding with an adaptive TTL,
  as Bitchat clamps 7 down to 5 in dense areas.
- **Routing between islands:** Spray-and-Wait copy budgets. For example, give
  at most N copies to data mules: vehicles, buses, people carrying boxes.
- **Routing over the backbone:** internet relays, in the style of Nostr
  relays.
- **Routing for directed traffic:** an optional coarse **geohash locator**
  ("deliver toward 9xj6") gives geographic forwarding with O(neighbours)
  state.
- **Legality filter:** before an object goes out on a link, the router
  checks it against the link's `may_encrypt` and `may_commercial` flags.

### Spaces and identity

- **Identity:** a keypair. The suite ID makes it future-proof.
- **Key rotation:** a *key event* object can rotate keys, add a post-quantum
  key, or record social recovery. For social recovery, k-of-n friends sign a
  re-key.
- **Commons:** public, signed, never encrypted, so it can ride ham bands
  legally.
- **Circles:** encrypted with a group key. Relays carry them blind: they can
  see the id and size, but not the content.
- **Archives:** content meant to last decades can have its Merkle root signed
  with a hash-based scheme (SLH-DSA). That costs ~8 KB once per batch, not
  per object.

## The floor ("CyberMesh Core")

This is what a node **must** implement to be a CyberMesh node. Everything else
is optional.

| Must | Why |
|---|---|
| One floor link: **ESP-NOW, 2.4 GHz, on a fixed network-wide channel**, LR when supported | Every $2–5 ESP32 has it, 2.4 GHz is licence-free worldwide, no extra chip is needed |
| Object format, suite 0x01 (Ed25519 + BLAKE3) | Verify anything from anyone |
| Set reconciliation over the commons index | Sync with any neighbor |
| Fountain-decode chunks; optionally encode them | Receive swarm transfers |
| SOS object type and priority | The one thing that must always get through |
| Store: ≥ a few MB of objects (flash or SD) **E** | Store-and-forward |
| Serve a captive-portal web page | Any phone can use the node with no app |

**Floor hardware target (E):** any ESP32 with Wi-Fi. A C3 or S3 is ideal, and
the AIO Board's **S2 is listed for LR**. That needs ~4 MB of flash and SD
optional. A screen is optional but useful for off-phone use.

**Why not LoRa as the floor?**

- It is the better long-range radio, but its bands differ by region (433,
  868 and 915 MHz).
- US 915 MHz rules are in flux right now.
- It needs an extra chip.

So LoRa is the **first recommended upgrade**, not the floor. The 2.4 GHz
versions (SX1280 FLRC at ~1 Mbps, and the LR2021) are attractive because the
band is the same everywhere.

**Open interop question:** ESP-NOW is Espressif's format. A Linux box with a
monitor-mode Wi-Fi card can probably send and receive the same vendor action
frames **E**. If it can, the floor isn't locked to one vendor's chips. This
needs testing early.

## Tech stack (recommended)

| Piece | Choice | Why |
|---|---|---|
| Spec | Plain-text versioned spec (CC BY), with test vectors | "Two strangers can build compatible nodes from the spec alone" |
| Reference core | **Rust, `no_std`, no allocator in the hot path.** Exposes a C ABI, compiles to WASM | It parses hostile radio input, so memory safety matters. The same core runs on ESP32, Pi, servers and in the browser |
| Second implementation | A small **C99** implementation of the floor only | Proves the spec is simple enough; also suits odd MCUs |
| ESP32 firmware | ESP-IDF (pinned tag), with the Rust core as a component **E** | Wi-Fi and ESP-NOW APIs live in IDF. Xtensa chips (S2/S3) need Espressif's Rust toolchain fork; RISC-V chips (C3/C5/C6) use upstream Rust. **Decide after a spike** |
| Host daemon | The same Rust core plus async I/O, for Pi, servers and laptops | Runs the internet, 802.11s, HaLow, ham (KISS/ARDOP/JS8), satellite and SD links |
| Crypto | Ed25519, X25519, BLAKE3, ChaCha20-Poly1305 or AES-GCM (hardware). Monocypher-class code in C, RustCrypto/dalek in Rust | Fast on ESP32 (~28 ms to verify). mbedTLS's 25519 is ~4x slower |
| Hash and chunks | BLAKE3 + Bao outboard trees | Verified streaming from any source |
| Sync | Negentropy-style RBSR; Willow 3D ranges later | Production-proven, bounded RAM |
| Coding | LT/online codes, or Wirehair if its license checks out | Swarm transfer without the RaptorQ patent risk |
| Compression | Tamp or heatshrink, plus static dictionaries per object type | Works in tiny RAM; dictionaries help short texts most |
| Phone UI | A captive-portal web app, very small (Preact-class), served from flash | iPhone works, no app store |
| Flashing | ESP Web Tools (Web Serial) | One-click install, as with Meshtastic and WLED |
| Interop gateways | Reticulum/LXMF, Meshtastic (MQTT), Nostr, DTN BPv7 | Talk to existing meshes; never depend on them |

**Why not build on Reticulum** (a change from the handoff):

- Its license added a field-of-use restriction, so it is no longer OSI
  open source.
- It has no crypto agility.
- Its global announce flooding won't scale to a planet.

Borrow its ideas: hash addressing, airtime-capped announces, interface
independence. Then bridge to it.

## Link roadmap by tier

| Tier | Links (floor in **bold**) | Notes |
|---|---|---|
| Pocket beacon | **ESP-NOW/LR** + SoftAP portal | Battery, leaf role |
| Rooftop relay | **ESP-NOW/LR** + LoRa (sub-GHz or 2.4 GHz FLRC) + directional antenna | Directional antennas are the cheapest range gain; respect EIRP limits |
| Home box | Pi + ESP32 "heart" over USB + Wi-Fi/802.11s + optional HaLow (US) | Archive, search, sneakernet spool |
| Hub | Server + hearts + internet + optional ham / satellite / broadcast | Bridges islands and continents |
| Future | Software-defined-radio PHY tier (post-2030, if certifiable) | Rides the same link-descriptor interface |

## What we would invent vs. borrow

**Borrow:**

- Content addressing (IPFS/Bao)
- Set reconciliation (Negentropy/Willow)
- Managed flooding and roles (Meshtastic/MeshCore)
- Announce budgets (Reticulum)
- Spool files (NNCP)
- Store-and-forward architecture (DTN)
- Onboarding (Meshtastic/WLED)

**Invent:** these combinations don't exist anywhere yet.

1. **Swarm transfer:** broadcast plus fountain-coded, content-addressed
   chunks, collected from any mix of senders and links.
2. **Legality-aware routing:** link descriptors carry legal flags, and the
   router enforces them.
3. **Fingerprint beacons:** sync state replaces chatty node beacons.
4. **One object format** that crosses a 5 bps HF link and a 10 Gbps fiber
   unchanged.
5. **A self-bootstrapping network:** the spec, firmware and guides are mesh
   objects, so the network can rebuild itself from any surviving node.

## Honest limits

- **Physics wins.** Long range and high speed trade off against each other.
  A $5 radio across a city will be kbps to hundreds of kbps, not Mbps. Speed
  comes from:
  - density
  - broadcast reuse
  - coding
  - directional antennas
  - wired and internet pipes
  - data mules (one SD card on a daily bus averages ~24 Mbps)
- **Phones can't relay in the background,** especially iPhones. Boxes do the
  syncing.
- **Nothing is deleted once it syncs.** The UI has to say so plainly.
- **Abuse in the public commons is a design problem, not a later
  moderation problem.** The design has to provide:
  - web-of-trust feeds
  - per-node refusal
  - shared blocklists
  - circles that relays can't read
- **Regulation varies by country.** Defaults must be legal by region. US
  915 MHz and EU HaLow are live examples.

## Revised roadmap

0. **Bench truth.** Before any spec, measure on real boards:
   - ESP-NOW v2 throughput at 1 Mbps, 11g, MCS rates and LR
   - LR range with stock and directional antennas
   - SoftAP + ESP-NOW coexistence on one channel
   - Ed25519/BLAKE3 timing on S2, S3 and C3
   - whether Linux can send and receive ESP-NOW frames

   *Done when:* a results table replaces the **E** numbers above.
1. **Spec v0.1:** objects, the floor frame format, reconciliation, swarm
   transfer, link descriptors.
   *Done when:* two people write compatible floor nodes from the spec alone.
   This means the Rust core plus the C floor implementation.
2. **Two boxes, one note**, as in the handoff.
3. **Swarm demo.**
   *Done when:* one box sends a 1 MB file to five boxes at least 3x faster
   than sequential unicast, measured.
4. **Three-hop relay plus a data mule.**
   *Done when:* a note crosses a gap that no radio spans, carried by a box in
   a car.
5. **Circles.**
6. **Home box and internet link.**
   *Done when:* two islands in different cities sync over the internet and
   keep working when it's unplugged.
7. **Ham link (cleartext, signed) and SD sneakernet spool.**
8. **Browser flasher, build guide, and the self-bootstrap pack.**

## Open questions for the owner

- Do you want the **floor to be ESP-NOW (2.4 GHz, every ESP32)**, as proposed?
  The alternative is to require LoRa in every node.
- Should the reference core be in **Rust (safer, compiles to WASM)**, or in
  **C (simpler toolchain, more contributors)**?
- Should the **public commons be on by default**, or **circles only**? This
  sets the abuse and legal exposure.
- **Where should the project live?** The handoff said its own repo; it lives
  here while it is a design.
