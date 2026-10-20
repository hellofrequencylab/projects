// CyberUnicorn sensor node: wake, read the I2C sensors and battery, broadcast
// one cu_sensor_packet_t per reading over ESP-NOW, deep sleep. Everything
// happens in setup(); loop() is never reached.
//
// Protocol: cyberpunkunicorn-os/apps/sensors/PROTOCOL.md. Contract:
// include/sensor_packet.h, a verbatim copy of the hub's.

#include <Arduino.h>
#include <atomic>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "config.h"
#include "sensor_packet.h"
#include "sensors.h"

static_assert(sizeof(NODE_NAME) - 1 <= CU_SENSOR_NAME_LEN, "NODE_NAME is longer than 12 characters");

static const uint8_t BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Survives deep sleep (not power loss or reset), so the hub's lost-packet
// count is meaningful across wakes.
RTC_DATA_ATTR static uint16_t seq;
RTC_DATA_ATTR static uint32_t wakes;

static std::atomic<int> sends_pending;

static void on_sent(const esp_now_send_info_t *, esp_now_send_status_t) { sends_pending--; }

static uint32_t node_id() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    return (uint32_t)mac[2] << 24 | mac[3] << 16 | mac[4] << 8 | mac[5];
}

static bool radio_start() {
    WiFi.persistent(false); // don't write Wi-Fi config to NVS on every wake
    WiFi.mode(WIFI_STA);
    if (esp_wifi_set_channel(CU_SENSOR_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
    if (esp_now_init() != ESP_OK) return false;
    esp_now_register_send_cb(on_sent);
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST, 6);
    peer.channel = 0; // current channel
    peer.ifidx = WIFI_IF_STA;
    return esp_now_add_peer(&peer) == ESP_OK;
}

static void sleep_now() {
    Serial.printf("sleep %d s\n", NODE_SLEEP_SECONDS);
    Serial.flush();
    esp_sleep_enable_timer_wakeup((uint64_t)NODE_SLEEP_SECONDS * 1000000ULL);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    wakes++;

    // Sensors and ADC before the radio: less self-heating, less supply noise.
    Reading r[8];
    int n = sensors_read(r, 7);
    float volts = battery_volts();
    uint8_t battery = CU_SENSOR_BATTERY_NONE;
    if (volts >= 0) {
        battery = battery_percent(volts);
        r[n++] = {CU_TYPE_VOLTAGE, CU_UNIT_VOLT, volts};
    }

    uint32_t id = node_id();
    Serial.printf("\ncu-node %08lx wake %lu: %d reading(s), battery %u\n", (unsigned long)id,
                  (unsigned long)wakes, n, battery);
    if (n == 0) {
        Serial.println("no sensor found on I2C and no battery pin; nothing to send");
        sleep_now();
    }
    if (!radio_start()) {
        Serial.println("ESP-NOW init failed");
        sleep_now();
    }

    for (int i = 0; i < n; i++) {
        cu_sensor_packet_t p;
        cu_sensor_packet_init(&p, id, seq++, r[i].type, r[i].unit, r[i].value, battery, NODE_NAME);
        sends_pending++;
        esp_err_t err = esp_now_send(BROADCAST, (const uint8_t *)&p, sizeof(p));
        if (err != ESP_OK) sends_pending--;
        Serial.printf("  seq %u type %u value %.2f %s\n", p.seq, p.type, p.value, err == ESP_OK ? "sent" : "FAILED");
    }
    // Sleeping with frames still queued drops them; the callback fires once
    // each has left the radio.
    uint32_t t0 = millis();
    while (sends_pending > 0 && millis() - t0 < 200) delay(1);
    sleep_now();
}

void loop() {}
