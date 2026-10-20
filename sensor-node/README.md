# sensor-node

Battery ESP32 sensor node for the CyberUnicorn 5000 hub. Wakes, reads a
BME280/BMP280 or SHT3x over I2C plus battery voltage, broadcasts the readings
over ESP-NOW on Wi-Fi channel 1, and deep-sleeps.

    ./build.sh                                  # build all boards -> build/<env>.bin
    ./build.sh esp32dev upload                  # build and flash a classic ESP32
    PLATFORMIO_CORE_DIR=.pio-core .venv/bin/pio device monitor -b 115200

Wiring, configuration and the end-to-end test with the hub: [`CLAUDE.md`](CLAUDE.md).
