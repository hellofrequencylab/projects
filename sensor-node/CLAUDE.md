# sensor-node

ESP32 sensor node that reports to the CyberUnicorn 5000 hub
(`cyberpunkunicorn-os/`, Sensors app) over ESP-NOW. Wake → read I2C sensors and
battery → one broadcast packet per reading → deep sleep.

## Pinned toolchain

- **PlatformIO Core 6.2.0**, in `.venv/` (`build.sh` creates it and refuses a
  different version).
- **Platform: pioarduino `55.03.39`** = Arduino-ESP32 **3.3.9** on ESP-IDF 5.5,
  the same release the hub's Bruce 1.16.1 build uses. Pinned by URL in
  `platformio.ini`.
- **Private core dir `.pio-core/`.** `build.sh` sets `PLATFORMIO_CORE_DIR` so the
  platform and toolchain live inside this project. Do not share `~/.platformio`:
  the hub's Bruce build patches `libnet80211.a` and swaps the Arduino libs
  package in there.
- Install + build: `./build.sh` (all envs) or `./build.sh <env>`. Output
  `build/<env>.bin`. First run fills `.pio-core/` (~5.6 GB) and took ~35 min
  here; later builds take ~1–2 min per env. If the first run stops at
  `Error: Python dependencies installation timed out` (the platform gives its
  pip step 300 s and some deps compile from source), run the install yourself
  once, then rerun `./build.sh`:
  `.pio-core/penv/bin/uv pip install --python=.pio-core/penv/bin/python "pioarduino>=6.1.19" "littlefs-python>=0.16.0" "fatfs-ng>=0.1.14" "pyyaml>=6.0.2" "rich-click>=1.8.6" "zopfli>=0.2.2" "intelhex>=2.3.0" "rich>=14.0.0" "cryptography>=45.0.3" "certifi>=2025.8.3" "ecdsa>=0.19.1" "bitstring>=4.3.1" "reedsolo>=1.5.3,<1.8" "esp-idf-size>=2.0.0" "esp-coredump>=1.14.0" "pyelftools>=0.32"`
  (list from `python_deps` in `.pio-core/platforms/espressif32/builder/penv_setup.py`).
- Flash: `./build.sh esp32dev upload [--upload-port /dev/cu.xxx]`.
- Monitor: `PLATFORMIO_CORE_DIR=.pio-core .venv/bin/pio device monitor -b 115200`
- Test: no unit tests; the test is the hub procedure below.

Envs (`platformio.ini`):

| env | board | SDA | SCL | battery ADC |
|---|---|---|---|---|
| `esp32dev` | any classic ESP32 DevKit | 21 | 22 | 34 (ADC1) |
| `aio-board-s2` | AIO Board V1.4's ESP32-S2 (see `aio-board/CLAUDE.md` for flashing) | 35 | 36 | none (no ADC1 pin broken out) |

## Grounding rules

- **The protocol is the hub's, not ours.** `include/sensor_packet.h` is a
  verbatim copy of `cyberpunkunicorn-os/apps/sensors/sensor_packet.h`
  (sha1 `318bf0ad…` at copy time). Never edit the copy. If the protocol needs to
  change, change it in the hub project (its own commit), then copy it out again.
  Check with `cmp include/sensor_packet.h ../cyberpunkunicorn-os/apps/sensors/sensor_packet.h`.
  Behaviour rules come from `cyberpunkunicorn-os/apps/sensors/PROTOCOL.md`.
- **ESP-IDF 5.5 changed ESP-NOW callbacks.** The send callback is
  `void (*)(const esp_now_send_info_t *, esp_now_send_status_t)`, not the old
  `(const uint8_t *mac, ...)`. Grep before touching radio code:
  `grep -rn esp_now_send_cb_t .pio-core/packages/framework-arduinoespressif32-libs/*/include/esp_wifi/include/esp_now.h`
- Sensor drivers in `src/sensors.cpp` are register-level from the datasheets
  (Bosch BME280 compensation formulas, Sensirion SHT3x single-shot + CRC). No
  sensor libraries, so nothing else to pin. Add a sensor the same way.

## Constraints that bite

- **`seq` lives in RTC memory.** It survives deep sleep, not reset or power
  loss; after a reset the hub sees `seq` jump back to 0 (counts as a restart,
  not loss).
- **Wait for the send callback before sleeping.** Deep sleep with frames queued
  silently drops them. `main.cpp` waits up to 200 ms.
- **Channel.** The node always sends on channel 1. The hub listens on channel 1
  only when it is *not* joined to Wi-Fi / running its AP; otherwise it listens on
  that network's channel and will not hear the node. Disconnect the hub's Wi-Fi
  for testing.
- **ADC2 is dead while Wi-Fi runs.** The battery pin must be ADC1 (ESP32:
  GPIO 32–39; ESP32-S2: GPIO 1–10). The node reads before starting Wi-Fi anyway.
- **GPIO 34–39 on the classic ESP32 are input-only, no pull-ups.** Fine for the
  battery divider; don't use them for I2C.
- `NODE_NAME` max 12 characters (static_assert). Empty = hub shows `node XXXX`.

## Wiring

I2C sensor (BME280/BMP280 breakout or SHT3x breakout, 3.3 V parts):

| sensor | ESP32 (`esp32dev`) | ESP32-S2 (`aio-board-s2`, "ESP32 LEADS OUT PINS") |
|---|---|---|
| VCC / VIN | 3V3 | 3V3 |
| GND | GND | GND |
| SDA | GPIO 21 | GPIO 35 |
| SCL | GPIO 22 | GPIO 36 |

Most breakouts already have 4.7–10 kΩ pull-ups on SDA/SCL; add them to 3V3 if
yours doesn't. Supported addresses: BME280/BMP280 `0x76`/`0x77` (SDO low/high),
SHT3x `0x44`/`0x45` (ADDR low/high). Several sensors at once are fine; each
reading is its own packet.

Battery (`esp32dev` only): two equal resistors (100 kΩ + 100 kΩ) in series
from battery + to GND, midpoint to **GPIO 34**. `NODE_BATTERY_DIVIDER` = 2.0.
Battery % uses a single-cell LiPo curve (3.30 V = 0 %, 4.20 V = 100 %); the
node also sends the voltage itself as a `voltage` reading. Note a DevKit's USB
chip and LDO draw milliamps in deep sleep; a real battery node wants a bare
module or a low-quiescent board.

## Configuration

`include/config.h`, overridable per env with `-D` in `platformio.ini`:
`NODE_NAME` (default empty), `NODE_SLEEP_SECONDS` (60), `NODE_I2C_SDA/SCL`,
`NODE_BATTERY_PIN` (-1 = none → battery byte 255), `NODE_BATTERY_DIVIDER`.

## Test procedure (hub)

1. Hub: make sure it is **not** joined to Wi-Fi or running its AP (so it
   listens on channel 1).
2. Flash the node: `./build.sh esp32dev upload`. Open the monitor. Each wake
   prints `cu-node <node_id> wake N: K reading(s), battery B`, then a line per
   packet (`seq … type … value … sent`) and `sleep 60 s`.
3. Hub serial CLI: `sensors listen 30`. While it listens, press **EN/RST** on
   the node to make it send immediately (otherwise it may sleep through the
   30 s window).
4. Expect one row per reading with the node's id (low 16 bits match the last 4
   hex digits the node printed), RSSI, battery % and lost = 0. BME280 → 3 rows
   (temperature °C, pressure hPa, humidity %); BMP280 → 2; SHT3x → 2; plus
   `voltage` if a battery pin is set.
5. Leave it for two cycles without resetting: `seq` should keep counting and
   the hub's lost count should stay 0. On the hub screen: Sensors → Live sensors
   shows the same rows marked `))`.

Status: both envs build with 0 warnings (esp32dev 939,168 B, aio-board-s2
860,480 B). **Not yet tested on hardware or against
a running hub.**
