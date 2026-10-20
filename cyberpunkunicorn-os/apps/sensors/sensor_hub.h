// Sensor hub: one list of channels fed by wired I2C chips and by ESP-NOW nodes,
// plus the CSV logger. Shared by the Sensors screens and the `sensors` CLI command,
// which run on different FreeRTOS tasks, so every public call takes the hub lock.
#pragma once

#include "i2c_sensors.h"
#include <Arduino.h>
#include <vector>

namespace sensors {

constexpr int MAX_CHANNELS = 24;
constexpr int MAX_WIRED = 12;
constexpr int HISTORY = 64;
constexpr uint32_t WIRELESS_STALE_MS = 5 * 60 * 1000;

struct Channel {
    char source[20]; // "BME280 76", or the node's name / "node 1A2B"
    char label[10];  // "Temp"
    uint8_t type, unit;
    float value;
    uint32_t updatedMs;

    bool wireless;
    uint32_t nodeId;
    uint8_t battery; // CU_SENSOR_BATTERY_NONE if unknown
    int8_t rssi;
    uint16_t seq;
    uint16_t lost; // packets missed, from sequence gaps

    float hist[HISTORY];
    uint8_t histCount, histHead;

    void push(float v);
    float histAt(int i) const; // 0 = oldest
};

// What a session found when it started.
struct SessionInfo {
    bool wiredOk = false;      // false if an SPI radio owns pins 22/27
    String warning;            // empty, or why wired sensors are off
    String hat;                // hat probe summary
    bool wirelessOk = false;
    int espnowChannel = 0;
};

// Unit and quantity text for the screen, CSV and CLI.
const char *unitText(uint8_t unit);
const char *typeText(uint8_t type);
String formatValue(float v, uint8_t unit); // sensible decimals for the unit

// Begins a session: probes the hat, scans and sets up the I2C bus, and (if
// `wireless`) starts the ESP-NOW receiver. Sessions nest (screen + CLI); only the
// first one touches hardware.
SessionInfo beginSession(bool wireless);
void endSession();
bool sessionActive();

// Reads chips that are due, takes in ESP-NOW packets, logs if it's time.
// Call often (every 10-50 ms) while a session is open.
void poll();

// Thread-safe copies.
int channelCount();
bool channel(int i, Channel &out);
std::vector<WiredDevice> wiredDevices();
SessionInfo sessionInfo();

// I2C scan report lines (also used by `sensors scan`).
std::vector<String> scanReport();

// Full text report: wired + wireless + log status (screen and CLI).
std::vector<String> readingsReport();

// Logging (NVS namespace "sensors").
bool loggingEnabled();
void setLoggingEnabled(bool on);
String logTarget(); // "SD:/sensors/log.csv", "LittleFS:/sensors.csv" or "none"

// Altimeter: baseline pressure for relative altitude (RAM only, until reboot).
float altimeterBaseline(); // 0 = not set
void setAltimeterBaseline(float hPa);

} // namespace sensors
