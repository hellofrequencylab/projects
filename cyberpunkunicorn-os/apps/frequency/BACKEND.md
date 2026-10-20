# Frequency device ↔ frequency-web

How the CyberUnicorn **Frequency** app relates to the Frequency loyalty
platform (`frequency-web`: Next.js + Supabase, prod `https://frequencylocal.com`).

> **Status (2026-10-19): the device endpoints below DO NOT EXIST in
> frequency-web yet.** They are a proposal. Everything under "Works today"
> needs no backend change. Reader mode is off by default. With it on, a 404
> from the server is shown as "Frequency backend not set up for devices yet".

## Works today: venue tags (no backend change)

Members check in with **their own phone**: they tap a venue tag, the phone opens
the link, and frequency-web credits the signed-in member (ledger credit,
"Regulars" stamp via `loyalty_record_visit`, +25 Zaps, hunts). The device only
**writes and verifies** those tags.

| Tag | URL written | Where the URL comes from |
|---|---|---|
| Space check-in node | `${SITE_URL}/n/<node uuid>?s=v1.<iat base36>.<tag>` | Space settings → Check-in (copy link). `tag` = base64url(first 16 bytes of HMAC-SHA256(server secret, `node-code:v1:<nodeId>:<iat>`)), so **only the server can mint it** |
| Event door | `${SITE_URL}/q/<7-char slug>?m=nfc` | the event's door code. The device adds `?m=nfc` if missing. **Never** on `/n/` links |

- One NDEF URI record per tag, NTAG213/215/216 only. A signed node URL is ~100
  characters, which fits NTAG213's 144 bytes.
- Checks before writing (warn, never block): http(s), `/n/` + UUID + `s=v1.<base36>.<22 base64url chars>`
  with no `m=`, or `/q/` + 7-char slug.
- Optional **permanent** write-lock (NTAG static + dynamic lock bits), behind
  two confirmations.
- Log of tags written: `/frequency/written.csv` on LittleFS
  (`ts,uid,kind,label,host+path,locked`). The `?s=` signature is left out.
- Pick list: `/frequency/tags.txt` (SD first, then LittleFS), one `label|url` per
  line, `#` comments. Upload it with the device web UI's file manager.
- Serial: `frequency tag <url> [lock-forever]`, `frequency read`, `frequency tags`, `frequency log`.

## Proposed: member cards + device scans

Not built server-side. The gap is that every `/api/v1` endpoint today credits
the **signed-in caller** (Bearer = Supabase user JWT; anything else is a 401).
There is no device auth, no member-card table and no NFC-UID mapping.

### Card format

NTAG21x with one NDEF URI record:

```
${SITE_URL}/c/<token>
```

`token` = 16 random bytes, base64url (22 chars), minted by the server and
opaque to the device. The device sends the tag UID with every scan so the
server can bind a token to the physical chip and reject copied cards. A phone
tapping the card opens `/c/<token>` (suggested: the member's own card page).

### Device auth

Every request carries

```
X-Frequency-Device: <device_id>.<device_secret>
Content-Type: application/json
```

`device_id` and `device_secret` are issued per device by the server (suggested:
a `devices` table tied to a Space / location, secret stored hashed). The device
keeps them in NVS (`frequency` namespace: `devid`, `devsecret`); set them in
Frequency → Settings or `frequency config devid <id>` / `frequency config devsecret <secret>`.
The device holds no other credential, so a stolen device can only check in
cards at its own Space.

### `POST {base}/api/v1/devices/scan`

Request:

```json
{ "card_token": "Q2hhbmdlTWVQbGVhc2UxMg", "uid": "04A1B2C3D4E5F6" }
```

Offline replays add (extension to the brief, optional for the server):

```json
{ "card_token": "…", "uid": "…", "queued": true, "scanned_at": 1792425600 }
```

`scanned_at` is epoch seconds and is sent only if the device clock was set
(NTP over Wi-Fi). The server decides whether a late scan still counts toward
"one credit per member per day".

Reply `200`:

```json
{
  "ok": true,
  "member": { "display_name": "Ava" },
  "zapsAwarded": 25,
  "loyalty": { "stamps": 4, "required": 8, "earned": false, "reward_code": null }
}
```

- `loyalty` may be absent or null (loyalty off for the Space / not a Business
  plan). `stamps` = stamps on the current card after this scan; `required` =
  3-12. When `earned` is true, `reward_code` is the 6-char code, and `stamps`
  may already have rolled over to 0 (the device then draws a full card).
- A scan that doesn't count (already checked in today, unknown/revoked card…):
  `200 { "ok": false, "reason": "Already checked in today" }`, or a `4xx` with
  the same body. The device shows `reason` as-is.

Device behaviour:

| Server says | Device does |
|---|---|
| `ok: true` | rainbow burst, name, "+N ZAPS", stamp card filling, reward explosion + code if `earned`. Then replays the offline queue |
| `ok: false` / `4xx` (not 404) | shows `reason`, drops the scan |
| `404` | "Frequency backend not set up for devices yet", drops the scan |
| `5xx`, timeout, no Wi-Fi | queues the scan (max 100, `/frequency/scanq.csv`), retried later |

### `POST {base}/api/v1/devices/cards`

Issue a member card. Request:

```json
{ "member_handle": "ava" }
```

Reply `200`:

```json
{ "ok": true, "token": "Q2hhbmdlTWVQbGVhc2UxMg", "url": "https://frequencylocal.com/c/Q2hhbmdlTWVQbGVhc2UxMg" }
```

The device writes `url` to a blank NTAG and reads it back. It doesn't report the
UID here: the first `scan` with that token is when the server binds the UID.
Errors use the same `ok: false` / `reason` shape.

### What the device never does

- No points, stamps or member list stored on the device: frequency-web is the
  source of truth. The device's only state is the offline scan queue and the
  write log.
- No minting of `/n/` signatures (the HMAC secret stays server-side).

## Settings (NVS namespace `frequency`)

| Key | Default | Meaning |
|---|---|---|
| `reader` | off | enables Member scan / Issue card |
| `base` | `https://frequencylocal.com` | frequency-web origin. `http://<mac-ip>:3000` works for local dev, but sends the device secret in clear text: LAN only |
| `devid` | empty | device id |
| `devsecret` | empty | device secret |

TLS: the device currently accepts any certificate (`setInsecure`). Pin the CA
once a production device API exists.

## Open questions

1. Does frequency-web want device scans at all, or should phones stay the only
   check-in path (venue tags)?
2. Device auth model: per-device secret (above) vs. a Space-scoped API key.
3. Card token ↔ UID binding: on first scan (above), or at issue time?
4. Should a late (queued) scan count for the day it happened (`scanned_at`) or
   the day it arrived?
5. Card page at `/c/<token>`: member profile, or a "claim this card" flow?
