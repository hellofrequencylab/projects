// Register-level drivers for common I2C sensors on the Grove port (SDA 27 /
// SCL 22). No libraries: each chip is a few registers, and this keeps flash small.
// Every call here expects the caller to hold the bus (acquireI2CBus()).
#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace sensors {

enum class Chip : uint8_t {
    Unknown, // answered, but we don't recognise it (see guessName())
    BME280,
    BMP280,
    SHT3x,
    AHT20,
    BH1750,
    SGP30,
    MPU6050,
    INA219,
    PN532,       // the hat's NFC reader: shares the bus, not a sensor
    Unsupported, // recognised chip without a driver yet (BME680, BMP180, ...)
};

// One value a chip produced. `label` is a short quantity name ("Temp").
struct Reading {
    const char *label;
    uint8_t type; // cu_sensor_type
    uint8_t unit; // cu_sensor_unit
    float value;
};
constexpr int MAX_READINGS = 4;

struct WiredDevice {
    Chip chip = Chip::Unknown;
    uint8_t addr = 0;
    char name[16] = {}; // "BME280", or "BME680?" for Unsupported
    uint32_t nextDue = 0;
    uint8_t phase = 0; // two-step chips: 0 = trigger next, 1 = result ready
    uint8_t errors = 0;

    // BME280/BMP280 calibration (datasheet section 4.2.2)
    struct {
        uint16_t T1;
        int16_t T2, T3;
        uint16_t P1;
        int16_t P2, P3, P4, P5, P6, P7, P8, P9;
        uint8_t H1, H3;
        int16_t H2, H4, H5;
        int8_t H6;
    } cal = {};
};

// Works out what is at `addr` from its ID registers. Leaves the chip set up for
// polling. Returns false only if the address stopped answering.
bool identify(TwoWire *w, uint8_t addr, WiredDevice &d);

// Name of a chip commonly found at `addr`, for things we can't read ("SSD1306?").
// nullptr if nothing comes to mind.
const char *guessName(uint8_t addr);

// Reads the chip if it is due. Returns the number of readings written to `out`
// (0 = nothing new this time), or -1 on a bus error.
int poll(TwoWire *w, WiredDevice &d, Reading out[MAX_READINGS], uint32_t nowMs);

// True for chips poll() can read.
inline bool readable(Chip c) { return c != Chip::Unknown && c != Chip::PN532 && c != Chip::Unsupported; }

} // namespace sensors
