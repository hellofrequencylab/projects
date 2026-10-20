// Register-level drivers for the node's I2C sensors. No libraries: each
// driver is a page of datasheet arithmetic and keeps the build pinned.
#pragma once
#include <stdint.h>

struct Reading {
    uint8_t type; // enum cu_sensor_type
    uint8_t unit; // enum cu_sensor_unit
    float value;
};

// Probe the bus and read every supported sensor found (BME280/BMP280 at
// 0x76/0x77, SHT3x at 0x44/0x45). Returns the number of readings written.
int sensors_read(Reading *out, int max);

// Battery voltage in volts, or a negative number if no battery pin is set.
float battery_volts();
// LiPo/Li-ion state of charge from resting voltage, 0-100.
uint8_t battery_percent(float volts);
