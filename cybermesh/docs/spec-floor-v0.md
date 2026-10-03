# CyberMesh floor protocol, v0

Status: **draft, bring-up only.** This document is normative for the code in
`core/`: where they disagree, the code is wrong (see `PLAN.md`, D4). v0 is
unsigned (suite 0x00), which means **any radio in range can inject notes**.
Don't deploy it beyond your own boxes. Suite 0x01 replaces it at milestone 2.

All integers are **little-endian**. Every frame is **≤ 250 bytes** (D2).

## 1. Link

- **Transport:** ESP-NOW **broadcast** (destination `ff:ff:ff:ff:ff:ff`),
  unencrypted, on 2.4 GHz **channel 1** (D1). Every frame is broadcast, and
  any node may use any frame it overhears.
- **Phone access:** a box also runs an open SoftAP on the same channel to
  serve phones. That is outside this protocol.

## 2. Objects

A v0 object is a **note**:

| Offset | Size | Field | Value |
|---|---|---|---|
| 0 | 1 | `obj_version` | `0x00` |
| 1 | 1 | `suite` | `0x00` (unsigned, SHA-256) |
| 2 | 1 | `type` | `0x01` (note) |
| 3 | 4 | `author` | node ID of the box where the note was posted |
| 7 | 8 | `time_ms` | author-claimed Unix time in ms; `0` = unknown |
| 15 | 1 | `text_len` | 1–200 |
| 16 | `text_len` | `text` | UTF-8, not NUL-terminated |

Rules:

- The object must be exactly `16 + text_len` bytes. Anything else is invalid.
- **Object ID** = SHA-256 of the object's bytes (32 bytes).
- **Short ID** = the first 8 bytes of the object ID.
- On the wire, objects are referenced by short ID.

## 3. Node state

- **Node ID:** a `u32`. The reference firmware uses the last 4 bytes of the
  Wi-Fi base MAC, read as little-endian.
- **Store:** a set of objects.
- **Fingerprint** of a store = the sum, modulo 2⁶⁴, of every short ID read as
  a little-endian `u64`. The empty store has count 0 and fingerprint 0.
- **Inventory order:** short IDs sorted ascending as `u64`.

## 4. Frames

Every frame starts with an 8-byte header:

| Offset | Size | Field | Value |
|---|---|---|---|
| 0 | 2 | `magic` | `0x43 0x4D` (`"CM"`) |
| 2 | 1 | `version` | `0x00` |
| 3 | 1 | `type` | see below |
| 4 | 4 | `src` | sender's node ID |

The body depends on the type:

| Type | Name | Body after header | Max frame size |
|---|---|---|---|
| `0x01` | BEACON | `count u16`, `fingerprint u64` | 18 |
| `0x02` | INV_REQ | `dest u32`, `start u16` | 14 |
| `0x03` | INV | `dest u32`, `start u16`, `total u16`, `n u8`, `n` × short ID (8 B), with `n ≤ 28` | 241 |
| `0x04` | GET | `dest u32`, `n u8`, `n` × short ID, with `n ≤ 29` | 245 |
| `0x05` | OBJ | one object (§2) | 224 |

A receiver **must** drop a frame when any of these hold:

- the magic is wrong
- the version is unknown
- `src` equals the receiver's own ID
- the body length doesn't match the type
- an OBJ carries an invalid object

## 5. Behavior

| When | Then |
|---|---|
| Every 5 s ± 1 s (random jitter) | Broadcast a BEACON with the node's own count and fingerprint |
| A BEACON from `src` has a non-zero count and (count, fingerprint) differs from the node's own, and no INV_REQ was sent to `src` in the last 3 s | Send INV_REQ(`dest=src`, `start=0`) |
| An INV_REQ arrives with `dest` = this node | Reply with INV: up to 28 short IDs starting at index `start` in inventory order, and `total` = store count |
| An INV arrives (from any sender, any `dest`) | GET(`dest=src`) the listed IDs this node doesn't hold, up to 29. If `dest` = this node and `start + n < total`, send INV_REQ(`dest=src`, `start + n`) after `300 ms + 25 ms × (IDs requested)` (immediately if none), so the sender's OBJ queue drains first |
| A GET arrives with `dest` = this node | Queue an OBJ for each requested ID the node holds |
| A valid OBJ arrives | If its short ID is new, store it |
| A note is posted locally | Store it. The next BEACON advertises it |
| The node's store changes (post or received OBJ) | The node **may** bring its next BEACON forward, but to no sooner than 200 ms from now. This lets notes cross hops in well under a second instead of 5 s per hop |

Pacing:

- Send at most **2 queued OBJ frames per 50 ms**.
- The OBJ queue holds at most 32 entries, with duplicates collapsed.

Multi-hop delivery needs no routing. A node that gains a note changes its
fingerprint, its neighbors notice that on its next beacon, and they pull the
note.

## 6. Known v0 limits (fixed by later milestones)

- **No authenticity:** anyone in range can inject or spoof notes. Fixed at
  milestone 2.
- **Bounded store:** the reference store holds 128 notes and evicts the
  oldest arrival when full. A full node's fingerprint can then keep
  disagreeing with its neighbors, so they re-fetch notes it just evicted.
- **Inventory exchange is linear,** not a range-based reconciliation, so it
  costs O(n) per sync. Fixed at milestone 3.
- **Slow loss recovery:** a lost GET or OBJ is retried only when the next
  BEACON shows the difference. In the simulator, 120 notes across 3 nodes
  take ~9 s with no loss and ~2 min at 20% frame loss.
- **No deletion and no edits.**
