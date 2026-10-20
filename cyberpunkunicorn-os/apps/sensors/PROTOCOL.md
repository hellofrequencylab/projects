# CyberUnicorn sensor protocol (v1)

How a sensor node sends readings to the CyberUnicorn 5000 hub. The contract is
[`sensor_packet.h`](sensor_packet.h): copy it into the node project unchanged.

## Transport

- **ESP-NOW**, broadcast (`FF:FF:FF:FF:FF:FF`) or unicast to the hub's MAC. No
  pairing, no router, no acknowledgement needed for broadcast.
- **Wi-Fi channel 1** (`CU_SENSOR_CHANNEL`). Exception: if the hub is joined to a
  Wi-Fi network (or running its AP), it listens on that network's channel
  instead. The Sensors screen footer and `sensors listen` show the channel.
- The hub only listens while the **Sensors** app is open (Live sensors or
  Altimeter), or during `sensors listen <s>` on the serial CLI. Bruce's own
  ESP-NOW file sharing uses the radio the rest of the time.
- Unencrypted. Don't send anything you mind a neighbour reading.

## Packet

One packet = one reading. 28 bytes, little-endian, no padding.

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 2 | `magic` | `0x5543` (bytes `43 55`, "CU") |
| 2 | 1 | `version` | `1` |
| 3 | 1 | `type` | what was measured, see below |
| 4 | 4 | `node_id` | unique per node; default: low 4 bytes of its MAC |
| 8 | 2 | `seq` | +1 per packet sent, wraps; the hub counts gaps as "lost" |
| 10 | 1 | `battery` | 0–100 %, or `255` = mains / not measured |
| 11 | 1 | `unit` | unit of `value`, see below |
| 12 | 4 | `value` | IEEE-754 float |
| 16 | 12 | `name` | label like `garden`; NUL-padded, may be empty |

A node with several sensors sends several packets with the same `node_id`, one
per `type`. The hub lists each (node, type, name) as its own row; the row's
source is `name`, or `node XXXX` (low 16 bits of `node_id`) if `name` is empty.

### Types and units

| type | meaning | unit |
|---|---|---|
| 0 | generic (give it a name) | any |
| 1 | temperature | 1 °C |
| 2 | humidity | 2 % |
| 3 | pressure (the Altimeter uses it) | 3 hPa |
| 4 | light | 4 lux |
| 5 | CO2 / eCO2 | 5 ppm |
| 6 | TVOC | 6 ppb |
| 7 | voltage | 7 V |
| 8 | current | 8 mA |
| 9 | power | 9 mW |
| 10 | acceleration | 10 g |
| 11 | rotation rate | 11 °/s |
| 12 | angle (tilt, heading) | 12 ° |
| 13 | altitude | 13 m |
| 14 | distance | 14 cm |
| 15 | soil moisture | 2 % |
| 16 | motion (0/1) | 0 none |
| 17 | count (events, pulses) | 0 none |

Unknown types show as "Value"; unknown units show blank.

### Versioning

- The hub accepts packets with the right `magic`, `version == 1` and at least
  28 bytes. Anything else is counted as "ignored" and dropped.
- v1 may grow by **appending** fields; old hubs ignore the extra bytes.
- Any change to existing fields bumps `CU_SENSOR_VERSION`.

## Hub behaviour

- Readings appear in **Sensors → Live sensors** next to wired I2C sensors,
  marked `))`, with the time since the last packet. A node silent for 5 minutes
  goes grey.
- When logging is on, every 10 s the hub appends each reading that changed to
  the CSV log (`link` column = `espnow`).
- The CLI `sensors` command prints the latest packets with node id, RSSI,
  battery and lost-packet count.

## Minimal node (Arduino-ESP32 3.x)

```cpp
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "sensor_packet.h"

const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint16_t seq = 0;
uint32_t nodeId;

void send(uint8_t type, uint8_t unit, float value) {
    cu_sensor_packet_t p;
    cu_sensor_packet_init(&p, nodeId, seq++, type, unit, value, CU_SENSOR_BATTERY_NONE, "garden");
    esp_now_send(BROADCAST, (const uint8_t *)&p, sizeof(p));
}

void setup() {
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(CU_SENSOR_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_now_init();
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST, 6);
    peer.channel = 0; // current channel
    esp_now_add_peer(&peer);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    nodeId = mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];
}

void loop() {
    send(CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, temperatureRead()); // ESP32 die temp, for testing
    delay(5000);
}
```

Battery nodes: send, then `esp_deep_sleep()`. Keep `seq` in RTC memory
(`RTC_DATA_ATTR uint16_t seq;`) so the hub's lost-packet count stays meaningful.
