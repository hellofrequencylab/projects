# CyberUnicorn 5000 remote protocol, v1

How the phone app (**Frequency Experience**, Android) controls a CyberUnicorn 5000
over USB serial or Bluetooth LE. Device side: `apps/remote/` + patch `0014`.
The phone side lives in `frequency-web`. Change both together.

## 1. Framing

- UTF-8 text, **one JSON object per line**, terminated by `\n`.
- **Phone → device:** `rpc {"id":<int>,"cmd":"<name>","args":{...}}`
  - `id`: any int the phone picks. The reply carries it back. Missing ids are treated as `0`.
  - `args`: an object. Can be left out, which means `{}`.
  - Keep a request line **≤ 512 bytes**. That's the BLE attribute limit and the web `/cm` packet size.
- **Device → phone:** exactly one reply per request:
  - `@rpc {"id":<same>,"ok":true,"data":{...}}`
  - `@rpc {"id":<same>,"ok":false,"error":"<code>","message":"<text>"}`
- **Unsolicited events:** `@evt {"event":"<name>","data":{...}}`
- **The phone must ignore every line that doesn't start with `@rpc ` or `@evt `.** Bruce
  prints logs on the same port (`COMMAND: …`, `[CLI] …`, a `# ` prompt after typed
  commands, the output of other CLI commands). The device writes `\n` *before* each
  protocol line as well as after it, so a protocol line never gets glued onto a
  partial log line. Expect blank lines and skip them. Also drop a line whose JSON
  doesn't parse: another task's log can, rarely, land inside an `@evt` line.
- Requests that can't be parsed get `@rpc {"id":null,"ok":false,"error":"bad_json",...}`.

### Error codes

| code | meaning |
|---|---|
| `bad_json` | the text after `rpc` isn't a JSON object (`id` is `null`) |
| `unknown_command` | no such `cmd` |
| `not_available` | a v1 command whose app isn't in this build yet (see §3), or an app hidden from the menu |
| `bad_args` | an arg is missing or out of range; `message` says which |
| `busy` | the device can't do that right now (e.g. the hat pins are in use); `message` says why |

### How `rpc` is wired (implementation note)

`rpc` is not a SimpleCLI command. SimpleCLI strips `"` and `\` from arguments,
which destroys JSON. Instead, patch 0014 makes `SerialCli::parse()` hand every raw
line to `cyberAppsHandleLine()` (`apps/hooks.h`) first. Every path into the Bruce
CLI goes through that function: USB serial, BLE serial, the web UI's
`POST /cm cmnd=…`, JS `serial.cmd()` and keyboard shortcuts. So `rpc` works on all
of them. The reply goes to the transport the line arrived on. Over web `/cm` the
HTTP reply only says "queued", and the `@rpc` line goes to the serial device.
Patch 0014 also stops `rpc` lines from sending the UI back to the main menu, which
Bruce otherwise does after every serial command except `nav`/`option`, and drops
the `# ` prompt after them.

### Concurrency

Send **one request at a time per transport, and wait for its `@rpc` reply** (time out
after ~5 s; `hat.probe` and `cli.run` can take a second or two). The device handles
lines one by one on its CLI task. On BLE a second write before the first is read
(every 10 ms) **overwrites** it, because the characteristic holds a single value.

## 2. Transports

### USB (OTG)

- The CYD's own USB port: a WCH **CH340** USB-serial chip (VID `0x1A86`, PID usually `0x7523`).
  It's the existing Bruce CLI: **115200 baud, 8N1, no flow control**.
- **Open the port with DTR = false and RTS = false, and never toggle them.**
  The CYD wires the CH340's DTR/RTS to the ESP32's EN and IO0 through the usual
  two-transistor auto-reset circuit. Asserting RTS on its own holds EN low, which
  resets the board, and a DTR/RTS transition during open glitches it too. Desktop
  serial tools reboot the board on open for this reason (`cyberpunkunicorn-os/CLAUDE.md`,
  constraint 3). With `usb-serial-for-android`: `port.open(conn)`,
  `port.setParameters(115200, 8, STOPBITS_1, PARITY_NONE)`, then
  `port.setDTR(false); port.setRTS(false)`.
- Nothing else is needed. From the board files: Bruce's CYD env runs the CLI on
  UART0 (pins 1/3) at 115200 with a 1 KB RX buffer, there's no USB-CDC to
  enumerate, and the CH340 needs no init sequence.
- **Power:** a phone's OTG port powers the CYD + hat (the hat's port has no data,
  see CLAUDE.md constraint 1). USB-C↔USB-C cables don't power the CYD (no CC
  pull-downs): use an OTG adapter that ends in USB-A, then a USB-A→USB-C cable to
  the CYD. Check the phone supplies enough current with the hat radios on.
- If the board does reset (cable wiggle, brown-out), it prints its boot logs and
  is ready in ~3–5 s (longer with the unicorn boot scene). Re-send `system.status`
  until it answers.
- The **GPS** app reuses UART0's pins (`GPS_SERIAL_TX/RX` = 3/1 on this board). While
  it runs, the USB CLI is gone.
- USB keeps working while Bluetooth is on. Patch 0014 polls USB too and answers
  on whichever transport the command came from.

### Bluetooth LE

Bruce's BLE API (`src/modules/ble_api/`) is reused as is. It's a NimBLE GATT server
with a single "serial" characteristic that the Bruce CLI reads from and prints to.

| Thing | Value |
|---|---|
| Advertised name | `CU5K` (patch 0014; the GAP device name is `CyberUnicorn`) |
| Serial service | `4371ec0b-3d43-49f9-b731-7c72a4a7bb91` (advertised, filter scans on it) |
| Serial characteristic | `d555ed97-bf2a-4f46-b3eb-d1fcdd7325e9`: READ \| WRITE \| NOTIFY, both directions |
| Battery service | `0x180F` / level `0x2A19` |
| Pairing | none (open; anyone in range can connect while it's on) |
| TX power | +9 dBm |

Phone side:

1. Scan for the serial service UUID, connect, then **request MTU 517**
   (`requestMtu(517)`). The device's NimBLE grants up to its configured max.
2. Enable notifications on the serial characteristic (CCCD `0x2902` = `01 00`).
3. Send each request as **one write** of the whole line `rpc {...}\n`. Use write
   *with* response: Android splits writes longer than MTU−3 into a prepared/long
   write, and NimBLE reassembles it (max 512 bytes) before the CLI sees it.
4. Replies come back as notifications of at most **MTU−3 bytes each**. NimBLE
   truncates longer notifications, so the device splits long lines itself.
   **Concatenate notifications and split on `\n`.** At the default MTU (23) a
   `system.status` reply is ~15 notifications at ~10 ms each. Request the big MTU.

While Bluetooth is on, Bruce routes *all* CLI output (`serialDevice`) to BLE, so
non-protocol text (output of other commands) shows up on the BLE stream too. Filter as in §1.

**On/off and RAM.** Turn it on from the device (Remote app → *Bluetooth now*), from
the CLI (`remote ble on`), or at boot (*Bluetooth at boot*, NVS namespace `remote`,
key `bleboot`, default off). Bruce warns that the BLE API needs a lot of RAM: the
NimBLE host plus controller take tens of KB. `remote ble on` refuses with less than
45 KB free. The heap before and after is logged on USB as
`[remote] BLE on: heap A -> B bytes`. **Not measured on hardware yet**: see the test
list in the commit/PR. Bruce's other Bluetooth apps (BLE menu: spam, scan, …)
re-init NimBLE and will tear the remote down. Turn the remote off first.

## 3. Commands (v1)

`*` = implemented in this build. The others answer `not_available` until their app
is merged. Each app then registers its handler in one line (§5).

### `system.status` *

```json
{"os":"CyberUnicorn 5000","version":"0.1.0","commit":"a1b2c3d","bruce_version":"1.16.1",
 "proto":1,"heap_free":81234,"heap_min":60123,"uptime_s":372,
 "wifi":{"connected":false,"ssid":"","ip":"","ap_ip":"192.168.4.1"},
 "ble":{"enabled":true,"connected":1,"at_boot":false},
 "hat":{"live":["pn532"],"probed_s_ago":12}}
```

- `wifi.connected/ssid/ip` refer to station mode. `ap_ip` is present only while the
  device runs its own AP (e.g. the WebUI startup app).
- `hat.live` comes from the **last** hat probe (`hat.probe`, the Hat app, or the
  `hat` CLI), any of `cc1101`, `nrf24`, `pn532`, `ir`, `ook433`. It's `[]` with
  `probed_s_ago: null` if nothing has probed since boot. Status never drives the
  shared pins itself.
- `commit` is the git commit the build came from (`build.sh`), with `-dirty` when it
  was built from uncommitted changes. `proto` is the protocol version.
- Extras beyond the brief: `heap_min`, `wifi.ap_ip`, `ble.connected`, `ble.at_boot`, `hat.probed_s_ago`.

### `system.caps` *

`{}` → `{"proto":1,"version":"0.1.0","commit":"a1b2c3d","commands":["system.status", ...]}`.
Every command this build answers. Grey out the rest instead of waiting for
`unknown_command`.

### `system.time` *

`{"epoch":1792526400}` → `{"epoch":1792526400}`. Sets the board's clock from the
computer's (epoch seconds, 2026 or later). The board has no battery clock and Bruce
boots with a made-up date, so **send this right after the hello**: a scan through
the cable carries no usable time until it has been set. `{}` reports the time the
board trusts (`0` = never set this boot).

### `apps.list` *

```json
{"apps":[{"name":"Light Show","hidden":false},{"name":"Hat","hidden":false},
         {"name":"Remote","hidden":false},{"name":"WiFi","hidden":false}, ...]}
```

Main-menu items in menu order. `hidden` means hidden via Config → Hide apps (and the
`TRIM_UNUSED_MENUS` defaults FM/LoRa/Ethernet). Hidden items aren't on screen.

### `screen.nav` *

`{"key":"next"|"prev"|"sel"|"esc"}` → `{"key":"next"}`. Injects one button press, the
same way Bruce's `nav` command and the web UI do it (`NextPress`… flag +
`AnyKeyPress` + `SerialCmdPress`). A dimmed screen stays dimmed. `select` is accepted
as an alias of `sel`.

### `cli.run` *

`{"line":"info"}` → `{"output":"…","result":true,"truncated":true?}`

Runs any Bruce CLI line and returns what it printed. Output is captured by swapping
Bruce's `serialDevice` for a buffer while the command runs. Limits:

- At most 2048 bytes are kept (`truncated:true` when cut).
- Text a command writes straight to `Serial` instead of `serialDevice` isn't
  captured. Example: `nav`'s "Next Pressed" echo and its options list.
- `result` is SimpleCLI's parse result (false for unknown commands, whose error
  text is in `output`).
- Like a typed command, anything except `nav …`/`option …` sends the UI back to the
  main menu afterwards.
- Long-running commands block the CLI (and all other `rpc`) until they finish.
- `rpc` can't be nested (`bad_args`).

### `hat.probe` *

```json
{"pn532":true,"i2c_other":[],"cc1101":false,"cc_partnum":255,"cc_version":255,
 "nrf24":false,"nrf_status":255,"nrf_setup_aw":255,"rx_checked":false,
 "rx_transitions":0,"rx_high_percent":0,"ir_likely":false,"ook_likely":false,
 "live_count":1,"summary":"PN532 (DIP 3)","advice":"Ready."}
```

All `HatProbeResult` fields (snake_case) plus `live_count`, `summary`, `advice`.
`busy` while Light Show runs, since the probe would drive the LED pin. Takes up to ~1 s.

### `lightshow.state` *

```json
{"running":true,"scene":3,"scene_name":"Seed of Life","level":1,"level_percent":8,
 "level_count":5,"pin":27,"scenes":["Singing Bowl","Soft Rain","…"]}
```

When not running, `scene`/`level` are the saved values the next start will use.
`pin` is the data pin (−1 = screen only).

### `lightshow.set` *

`{"scene"?: <index> | "<name>", "level"?: 0-4, "running"?: true|false}` → same shape
as `lightshow.state`, plus `"starting":true` when it asked the UI to open the show.

- While running, scene and level change on the next frame (~15 ms) and are saved,
  exactly like the buttons. `running:false` stops the show and returns to the menu.
- While stopped, scene and level are saved for the next start. `running:true` opens
  Light Show on the device's UI. This works **only from the main menu**: otherwise
  `busy` (send `screen.nav esc` a few times to get back to the main menu, then retry). The show
  may still refuse to start if a hat module is powered on its data pin. It then
  emits `lightshow.error` and shows a warning on screen.
- Scene names match case-insensitively. Level steps are 4/8/15/25/40 % brightness.

### Wi-Fi (`apps/net/`)

So the console on a computer can put the board on a network over USB, with no
typing on the touch keyboard. Each blocks the CLI task while it runs.

| cmd | args | data |
|---|---|---|
| `wifi.status` | `{check?: bool}` | `{connected, ssid, ip, rssi?, channel?, mac, ap_ip?, saved:[ssid…], internet?, internet_detail?}`. `saved` is names only, never passwords. `check:true` adds the internet probe below (up to ~5 s). |
| `wifi.scan` | `{}` | `{networks:[{ssid, rssi, channel, auth, saved}]}`, up to 30. `auth` is `open`, `wep`, `wpa2`, `wpa3`, `enterprise` (the board can't join these) or `other`; `ssid:""` is a hidden network. ~2-4 s. |
| `wifi.join` | `{ssid, password?, save?: true, timeout_ms?: 20000}` | `{ssid, ip, saved, internet, portal?}`. No password = open network. Emits `wifi.joining` first. Fails `not_found`, `wrong_password`, `timeout`, `low_memory` or `bad_args`. Turns the Bluetooth remote off first if Wi-Fi won't fit beside it (no PSRAM). Keeps the board's own hotspot up if it is on. |
| `wifi.forget` | `{ssid}` | `{ssid}`; `not_saved` if it wasn't |

`internet` is `online` (the probe `http://connectivitycheck.gstatic.com/generate_204`
got its 204), `captive` (something else answered: a guest network's sign-in page;
`portal` / `internet_detail` is its address when it sent one), `no_internet`, or
`no_wifi`. The board has no browser, so on `captive` tell the person to use another
network or a phone hotspot.

Serial equivalents: `wifi add "<ssid>" ["<password>"]`, `wifi forget "<ssid>"`
(patch 0015), `netcheck`.

### Frequency tags (`apps/remote/app_handlers.cpp`)

Each waits for a tap (`timeout_ms`, default 10000, max 30000) and emits
`frequency.waiting_for_tag` first. A card is
`{uid, type, ntag, locked, capacity, uri, kind}`, `kind` being `venue check-in`,
`event door`, `member card`, `other` or `empty`.

| cmd | args | data |
|---|---|---|
| `frequency.read_tag` | `{timeout_ms?}` | the card |
| `frequency.write_tag` | `{url, label?, timeout_ms?}` | the card + `written` (host + path, no signature). Fails `bad_url`, `write_failed`. |
| `frequency.scan` | `{timeout_ms?}` | `{card, member:{display_name}, zapsAwarded, loyalty?:{stamps, required, earned, reward_code?}}`, or `{card, relay}` through the cable (below). Fails `not_member_card`, `reader_mode_off`, `backend_not_set_up`, `offline` (queued) or `rejected` (`message` = the server's reason code). |

All three fail `no_reader` (PN532 not answering: DIP 3), `busy`, `timeout` or `card_error`.

### Setup over the cable (`apps/frequency/provision.cpp`)

Makes a board a Space's counter scanner without typing anything on it (plan
Phase 3). The website registers the scanner (the owner action in Check-in &
scanners), sends the id and secret straight to the board, then puts it on Wi-Fi
with `wifi.scan` + `wifi.join`.

| cmd | args | data |
|---|---|---|
| `frequency.config` | `{}` | `{reader, base, device_id, secret_set, ready, queued, clock_set, cable}`. The secret is never sent back. `ready` = reader on with an id and secret. `cable` = a scan right now would go through this computer (no Wi-Fi, clock set, ready). |
| `frequency.provision` | `{device_id, secret, base?, reader?: true}` | same as `frequency.config`. `device_id` is the scanner's uuid, `secret` its 43-character base64url secret, `base` the site origin (default `https://frequencylocal.com`). Provisioning a different `device_id` drops the old one's offline queue. `bad_args` says which field is wrong. |
| `frequency.unprovision` | `{}` | same as `frequency.config`. Forgets the id and secret, turns reader mode off and drops the offline queue. Use it when the owner revokes the scanner with the board plugged in. |

The board stores the secret in NVS (namespace `frequency`, key `devsecret`).
Without flash encryption anyone holding the board can read it; that is the
threat model frequency-web's LOYALTY §7 accepts (a stolen secret checks in only
that Space's cards, once per member per day, until revoked).

**The serial log never shows an `rpc` line's arguments.** Bruce echoes every line
it reads as `COMMAND: <line>`; patch 0014 prints just `COMMAND: rpc` for these,
because `frequency.provision` and `wifi.join` carry secrets.

### Scans through the cable (`apps/frequency/backend.cpp`)

A board with no Wi-Fi, plugged into a computer whose device page is open, checks
members in through that computer (plan Phase 5). **The secret never leaves the
board:** the board signs the scan, and the computer forwards the exact bytes.

The board uses the cable when all of these hold: no Wi-Fi, the computer sent an
`rpc` line over USB in the last **30 s** (send `system.status` every ~10 s while
the page is open), `system.time` has been set this boot, and the scanner is
provisioned. Otherwise it queues the scan as before.

1. A member taps a card on the Frequency home screen. The board emits
   `@evt {"event":"frequency.scan_relay","data":{"nonce":"9f86d081","device_id":"<uuid>","body":"<json>","signature":"v1=<hex>"}}`.
   `body` is a JSON **string**, for example
   `{"card_token":"Q2hh…","uid":"04A1B2C3D4E5F6","scanned_at":1792526400,"nonce":"9f86d081"}`.
2. The computer POSTs `body` byte for byte to `{base}/api/v1/devices/scan` with
   `Content-Type: application/json`, `X-Frequency-Device-Id: <device_id>` and
   `X-Frequency-Signature: <signature>`. No `X-Frequency-Device` header: the
   computer doesn't have the secret.
3. It sends the answer back: `rpc {"id":7,"cmd":"frequency.relay_reply","args":{"nonce":"9f86d081","status":200,"body":{...the server's JSON...}}}`.
   `status` is the HTTP status, or `0` if the computer couldn't reach the server
   (the board then queues the scan). Keep the line under 512 bytes: the scan reply
   is small. A reply after the board stopped waiting (12 s) fails `not_waiting`.
4. The board shows the result exactly as for a Wi-Fi scan.

`rpc frequency.scan` over the cable with no Wi-Fi works the same way, but in the
reply: its data is `{card, relay:{nonce, device_id, body, signature}}` and the
computer posts it and keeps the answer (the board can't wait for a second command
while it is still answering the first).

**Signature** (what the server checks):

```
key       = sha256(secret)                       (the 32 raw bytes; the server stores this hash as hex)
message   = "scan:v1:" + device_id + ":" + body  (body = the exact bytes posted)
signature = "v1=" + hex(HMAC-SHA256(key, message))
```

The server then rejects a `scanned_at` more than a few minutes from its own clock,
and a `nonce` it has already seen from that device within the window, so a
captured request can't be replayed. Keying on the stored hash means a database
leak could forge cable scans for that Space, which is the same blast radius as a
leaked device secret; revoke the scanner either way.

### Planned commands (not in this build)

Handlers are wired in by the coordinator when each app branch merges. Args and data
as agreed for v1:

| cmd | args | data |
|---|---|---|
| `led.set` | `{r,g,b}` 0-255 | `{r,g,b}` |
| `led.mode` | `{mode}`: proposed `"off"`, `"solid"`, `"rainbow"`, `"breathe"` (the LED app's modes) | `{mode}` |
| `sound.tone` | `{hz, ms}` | `{hz, ms}` (returns once the tone has started) |
| `light.read` | `{}` | `{raw, percent}` (CYD LDR on GPIO 34) |
| `sensors.read` | `{}` | `{sensors:[{name, kind, value, unit}]}`, e.g. `{"name":"AHT20","kind":"temperature","value":21.4,"unit":"C"}` |

Exact field names for these get fixed by their app branches. Each branch must update
this table when it registers its handler.

### Events

| event | data | when |
|---|---|---|
| `lightshow.state` | same as `lightshow.state` | show starts, stops, or changes scene/level (buttons or remote) |
| `lightshow.error` | `{message}` | the show refused to start |
| `wifi.joining` | `{}` | `wifi.join` started (it can take up to 30 s) |
| `hello` | `{proto, os, fw, commit, board}`, e.g. `{"proto":1,"os":"CyberUnicorn 5000","fw":"0.1.0","commit":"a1b2c3d","board":"cyd-2432s028r"}` | once, at the end of boot (~18 s after the port opened, since opening it resets the board). Then send `system.time`. |
| `frequency.waiting_for_tag` | `{}` | a `frequency.*` command is waiting for a tap |
| `frequency.scan_relay` | `{nonce, device_id, body, signature}` | a card was tapped with no Wi-Fi and this computer on the cable: forward it (Scans through the cable) |

Events go to USB always, and to BLE when a phone is connected.

## 4. On the device

- **Remote** main-menu app: *Status* (live USB/BLE state, free RAM, last 8 commands;
  SEL toggles Bluetooth), *Bluetooth now*, *Bluetooth at boot*, *How to connect*.
- CLI: `remote` (status + recent commands), `remote ble on|off`, `remote boot on|off`.

## 5. Adding a command

In your app's `.cpp`:

```cpp
#include "../remote/remote.h"

REMOTE_COMMAND("led.set", [](remote::Request &r) {
    int red = r.args["r"] | -1;
    if (red < 0 || red > 255) return r.fail("bad_args", "r is 0-255");
    // ... do it ...
    r.data["r"] = red;
});
```

Handlers run on the CLI task (8 KB stack), not the UI task. Don't draw on the
screen from a handler: leave a request for the app's own loop
(`apps/lightshow/LightShowMenu.cpp` shows the pattern). To send an event, call
`remote::emit("name", obj)`.

## 6. Internet relay (website side built, device side not)

frequency-web now has `POST /api/v1/devices/realtime-token` (LIVE-1155), behind
`DEVICE_RELAY_ENABLED`. Its allow-list includes `led.mode`, which this firmware
answers `not_available`. The device-side Realtime client below is not built yet.

Goal: control the device from any network. Neither side needs an open port. Both
are clients of **Supabase Realtime** (frequency-web's backend), meeting on a
private channel per device.

```
phone app ──(user session JWT)──► Realtime channel "device:<id>" ◄──(device token)── CyberUnicorn
                                   broadcast "rpc" / "@rpc" / "@evt"
```

### Device side

1. **Identity.** The device already has `X-Frequency-Device: <uuid>.<secret>` from
   the frequency-web device API (session `fw-devices`, branch `live/nfc-device-checkin`:
   `POST /api/v1/devices/scan`, `/devices/cards`). The relay reuses it. Nothing new
   is stored on the device.
2. **Token.** With Wi-Fi up (station mode) and *Internet remote* switched on (new NVS
   key `remote/relay`, default off), the device calls
   `POST /api/v1/devices/realtime-token` with that header. The reply is
   `{ok, token, expires_at, channel:"device:<uuid>", realtime_url}`: a Supabase-signed
   JWT that lives ~15 min, with `role: authenticated`, `sub: device:<uuid>` and a
   claim limiting it to that one channel. The device re-mints it ~2 min before
   expiry, or on a 401. Same flat `{ok, reason}` error replies and `Retry-After` as the
   existing device API.
3. **Connection.** A WebSocket (`wss://<project>.supabase.co/realtime/v1/websocket?apikey=<anon>&vsn=1.0.0`)
   using the ESP32 core's `WiFiClientSecure` plus a small Phoenix-protocol client:
   `phx_join` on `realtime:device:<uuid>` with `config.private=true` and
   `access_token=<token>`, a `heartbeat` every 25 s, and `access_token` pushes on
   refresh. A minimal client is ~10 KB of flash. TLS costs ~40 KB of heap while
   connected, so **BLE and the relay shouldn't run at the same time** on this board.
   The Remote app enforces one or the other.
4. **Payloads: the same protocol, untouched.** The phone broadcasts event `rpc` with
   payload `{line:"rpc {...}"}`. The device passes `line` to the same
   `remote::handleLine()` with a third "relay" transport (a `SerialDevice` that buffers
   the reply line) and broadcasts event `reply` with `{line:"@rpc {...}"}`. `@evt`
   lines go out as event `evt`. The phone reuses its line parser unchanged.
5. **Safety.** The relay accepts only `rpc` lines (no raw CLI), and `cli.run` is
   **disabled over the relay**: it would be remote shell access. Only an explicit
   allow-list goes over the internet: `system.status`, `apps.list`, `lightshow.*`,
   `led.*`, `sound.tone`, `light.read`, `sensors.read`, `frequency.read_tag`. A device
   setting can narrow it further. The screen shows a "remote: internet" badge while
   joined.
6. **Presence.** The device `track()`s `{online:true, version}` on the channel, so the
   phone can show online/offline without polling.

### What frequency-web needs to add

1. **`POST /api/v1/devices/realtime-token`**: authenticates `X-Frequency-Device` like
   `/devices/scan`, and returns a short-lived JWT signed with the project's JWT
   secret (server-side only) for `sub: device:<uuid>`, carrying a custom claim
   `device_id`. Rate-limited like the other device routes.
2. **Realtime authorization (RLS on `realtime.messages`)** for private channels:
   - a device JWT may `select`/`insert` only on topic `device:<its device_id>`;
   - a user JWT may do the same only for devices they own or manage: a join from
     `realtime.topic()` → `device_id` → the existing devices table → the venue/owner
     the user belongs to.
3. **Device ownership / pairing**: a way to link a device to a user or venue if the
   devices table doesn't already have one (it's created by the device API branch).
   The phone app needs `GET /api/v1/me/devices` to list what it can control.
4. **A device setting row** (`relay_enabled`, `relay_allowed_cmds`) so owners can
   switch the relay off server-side. The token endpoint refuses when disabled.
5. **Audit log** (optional): a table of relayed commands (device, user, cmd, ok, at),
   written by the phone app or an Edge Function. Payloads stay on Realtime, not in the DB.
6. Feature flag `DEVICE_RELAY_ENABLED`, off by default, like `SCANNER_DEVICES_ENABLED`.
