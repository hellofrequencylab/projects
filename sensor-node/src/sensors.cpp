#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "sensor_packet.h"

// ---- I2C helpers ----

static bool i2c_present(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

static bool i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, len) != len) return false;
    for (size_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

// ---- BME280 / BMP280 (Bosch datasheet BST-BME280-DS002, section 4.2.3) ----

#define BME_REG_ID 0xD0
#define BME_REG_RESET 0xE0
#define BME_REG_CTRL_HUM 0xF2
#define BME_REG_STATUS 0xF3
#define BME_REG_CTRL_MEAS 0xF4
#define BME_REG_DATA 0xF7
#define BME_CHIP_BME280 0x60
#define BME_CHIP_BMP280 0x58

static int bme280_read(uint8_t addr, Reading *out, int max) {
    uint8_t id;
    if (!i2c_read_regs(addr, BME_REG_ID, &id, 1)) return 0;
    if (id != BME_CHIP_BME280 && id != BME_CHIP_BMP280) return 0;
    bool humidity = id == BME_CHIP_BME280;

    // Soft reset clears whatever a previous boot left configured, then wait
    // for the NVM calibration copy (status bit 0) to finish.
    i2c_write_reg(addr, BME_REG_RESET, 0xB6);
    delay(3);
    for (int i = 0; i < 10; i++) {
        uint8_t st;
        if (i2c_read_regs(addr, BME_REG_STATUS, &st, 1) && !(st & 0x01)) break;
        delay(1);
    }

    uint8_t c[26];
    if (!i2c_read_regs(addr, 0x88, c, sizeof(c))) return 0;
    uint16_t T1 = c[0] | c[1] << 8;
    int16_t T2 = c[2] | c[3] << 8, T3 = c[4] | c[5] << 8;
    uint16_t P1 = c[6] | c[7] << 8;
    int16_t P2 = c[8] | c[9] << 8, P3 = c[10] | c[11] << 8, P4 = c[12] | c[13] << 8;
    int16_t P5 = c[14] | c[15] << 8, P6 = c[16] | c[17] << 8, P7 = c[18] | c[19] << 8;
    int16_t P8 = c[20] | c[21] << 8, P9 = c[22] | c[23] << 8;
    uint8_t H1 = c[25];
    int16_t H2 = 0, H4 = 0, H5 = 0;
    uint8_t H3 = 0;
    int8_t H6 = 0;
    if (humidity) {
        uint8_t h[7];
        if (!i2c_read_regs(addr, 0xE1, h, sizeof(h))) return 0;
        H2 = h[0] | h[1] << 8;
        H3 = h[2];
        H4 = (int16_t)((int8_t)h[3] * 16 | (h[4] & 0x0F));
        H5 = (int16_t)((int8_t)h[5] * 16 | (h[4] >> 4));
        H6 = (int8_t)h[6];
        i2c_write_reg(addr, BME_REG_CTRL_HUM, 0x01); // humidity x1; latched by ctrl_meas
    }
    // Temperature x1, pressure x1, forced mode: one measurement, then sleep.
    i2c_write_reg(addr, BME_REG_CTRL_MEAS, 0x25);
    delay(10); // max 9.3 ms at x1/x1/x1
    for (int i = 0; i < 20; i++) {
        uint8_t st;
        if (i2c_read_regs(addr, BME_REG_STATUS, &st, 1) && !(st & 0x08)) break;
        delay(1);
    }

    uint8_t d[8];
    if (!i2c_read_regs(addr, BME_REG_DATA, d, humidity ? 8 : 6)) return 0;
    int32_t adc_P = (int32_t)d[0] << 12 | d[1] << 4 | d[2] >> 4;
    int32_t adc_T = (int32_t)d[3] << 12 | d[4] << 4 | d[5] >> 4;
    int32_t adc_H = humidity ? (d[6] << 8 | d[7]) : 0;
    if (adc_T == 0x80000) return 0; // temperature skipped: measurement never ran

    int32_t v1 = ((((adc_T >> 3) - ((int32_t)T1 << 1))) * T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)T1) * ((adc_T >> 4) - (int32_t)T1)) >> 12) * T3) >> 14;
    int32_t t_fine = v1 + v2;
    int n = 0;
    if (n < max) out[n++] = {CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, ((t_fine * 5 + 128) >> 8) / 100.0f};

    if (adc_P != 0x80000) {
        int64_t p1 = (int64_t)t_fine - 128000;
        int64_t p2 = p1 * p1 * P6;
        p2 = p2 + ((p1 * P5) << 17);
        p2 = p2 + ((int64_t)P4 << 35);
        p1 = ((p1 * p1 * P3) >> 8) + ((p1 * P2) << 12);
        p1 = (((int64_t)1 << 47) + p1) * P1 >> 33;
        if (p1 != 0) {
            int64_t p = 1048576 - adc_P;
            p = (((p << 31) - p2) * 3125) / p1;
            p1 = ((int64_t)P9 * (p >> 13) * (p >> 13)) >> 25;
            p2 = ((int64_t)P8 * p) >> 19;
            p = ((p + p1 + p2) >> 8) + ((int64_t)P7 << 4); // Pa in Q24.8
            if (n < max) out[n++] = {CU_TYPE_PRESSURE, CU_UNIT_HPA, (uint32_t)p / 25600.0f};
        }
    }

    if (humidity && adc_H != 0x8000) {
        int32_t h = t_fine - 76800;
        h = (((((adc_H << 14) - ((int32_t)H4 << 20) - ((int32_t)H5 * h)) + 16384) >> 15) *
             (((((((h * H6) >> 10) * (((h * H3) >> 11) + 32768)) >> 10) + 2097152) * H2 + 8192) >> 14));
        h = h - (((((h >> 15) * (h >> 15)) >> 7) * H1) >> 4);
        h = h < 0 ? 0 : h;
        h = h > 419430400 ? 419430400 : h;
        if (n < max) out[n++] = {CU_TYPE_HUMIDITY, CU_UNIT_PERCENT, (uint32_t)(h >> 12) / 1024.0f};
    }
    return n;
}

// ---- SHT3x (Sensirion SHT3x-DIS datasheet, sections 4.3 and 4.12) ----

static uint8_t sht_crc(const uint8_t *d) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++) {
        crc ^= d[i];
        for (int b = 0; b < 8; b++) crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }
    return crc;
}

static int sht3x_read(uint8_t addr, Reading *out, int max) {
    // Single shot, high repeatability, no clock stretching.
    Wire.beginTransmission(addr);
    Wire.write(0x24);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return 0;
    delay(16); // max 15.5 ms
    uint8_t d[6];
    if (Wire.requestFrom(addr, (size_t)6) != 6) return 0;
    for (int i = 0; i < 6; i++) d[i] = Wire.read();
    if (sht_crc(d) != d[2] || sht_crc(d + 3) != d[5]) return 0;

    uint16_t rt = d[0] << 8 | d[1], rh = d[3] << 8 | d[4];
    int n = 0;
    if (n < max) out[n++] = {CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, -45.0f + 175.0f * rt / 65535.0f};
    if (n < max) out[n++] = {CU_TYPE_HUMIDITY, CU_UNIT_PERCENT, 100.0f * rh / 65535.0f};
    return n;
}

int sensors_read(Reading *out, int max) {
    Wire.begin(NODE_I2C_SDA, NODE_I2C_SCL, 100000);
    int n = 0;
    for (uint8_t addr : {0x76, 0x77})
        if (i2c_present(addr)) n += bme280_read(addr, out + n, max - n);
    for (uint8_t addr : {0x44, 0x45})
        if (i2c_present(addr)) n += sht3x_read(addr, out + n, max - n);
    Wire.end();
    return n;
}

// ---- Battery ----

float battery_volts() {
#if NODE_BATTERY_PIN < 0
    return -1.0f;
#else
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(NODE_BATTERY_PIN);
    return mv / 8 / 1000.0f * NODE_BATTERY_DIVIDER;
#endif
}

uint8_t battery_percent(float v) {
    // Typical single-cell LiPo resting curve, linear between points.
    static const float volts[] = {3.30f, 3.50f, 3.60f, 3.70f, 3.75f, 3.80f, 3.85f, 3.95f, 4.05f, 4.20f};
    static const uint8_t pct[] = {0, 5, 10, 20, 30, 40, 50, 65, 80, 100};
    const int n = sizeof(pct);
    if (v <= volts[0]) return 0;
    if (v >= volts[n - 1]) return 100;
    for (int i = 1; i < n; i++)
        if (v < volts[i])
            return pct[i - 1] + (uint8_t)((v - volts[i - 1]) / (volts[i] - volts[i - 1]) * (pct[i] - pct[i - 1]));
    return 100;
}
