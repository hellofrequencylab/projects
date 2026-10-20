#include "i2c_sensors.h"

#include "sensor_packet.h"
#include <math.h>

namespace sensors {

namespace {

// How often each chip is read once it's running.
constexpr uint32_t READ_INTERVAL_MS = 500;
constexpr uint32_t SGP30_INTERVAL_MS = 1000; // its baseline algorithm wants exactly 1 Hz

bool writeBytes(TwoWire *w, uint8_t addr, const uint8_t *b, size_t n) {
    w->beginTransmission(addr);
    w->write(b, n);
    return w->endTransmission() == 0;
}

bool writeReg(TwoWire *w, uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t b[2] = {reg, val};
    return writeBytes(w, addr, b, 2);
}

bool command16(TwoWire *w, uint8_t addr, uint16_t cmd) {
    uint8_t b[2] = {uint8_t(cmd >> 8), uint8_t(cmd & 0xFF)};
    return writeBytes(w, addr, b, 2);
}

bool readRaw(TwoWire *w, uint8_t addr, uint8_t *buf, size_t n) {
    if (w->requestFrom(addr, n) != n) return false;
    for (size_t i = 0; i < n; i++) buf[i] = w->read();
    return true;
}

bool readRegs(TwoWire *w, uint8_t addr, uint8_t reg, uint8_t *buf, size_t n) {
    w->beginTransmission(addr);
    w->write(reg);
    if (w->endTransmission() != 0) return false;
    return readRaw(w, addr, buf, n);
}

// Sensirion CRC-8 (poly 0x31, init 0xFF), also used by the AHT20.
uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

// Checks `words` Sensirion words (2 bytes + CRC each).
bool sensirionOk(const uint8_t *buf, int words) {
    for (int i = 0; i < words; i++)
        if (crc8(buf + i * 3, 2) != buf[i * 3 + 2]) return false;
    return true;
}

void setName(WiredDevice &d, const char *n) { strlcpy(d.name, n, sizeof(d.name)); }

uint16_t le16(const uint8_t *b) { return b[0] | (b[1] << 8); }

// ---- BME280 / BMP280 ---------------------------------------------------------

bool bmeSetup(TwoWire *w, WiredDevice &d) {
    writeReg(w, d.addr, 0xE0, 0xB6); // soft reset
    delay(5);
    uint8_t c[26];
    if (!readRegs(w, d.addr, 0x88, c, 24)) return false;
    auto &k = d.cal;
    k.T1 = le16(c), k.T2 = le16(c + 2), k.T3 = le16(c + 4);
    k.P1 = le16(c + 6), k.P2 = le16(c + 8), k.P3 = le16(c + 10), k.P4 = le16(c + 12);
    k.P5 = le16(c + 14), k.P6 = le16(c + 16), k.P7 = le16(c + 18), k.P8 = le16(c + 20);
    k.P9 = le16(c + 22);
    if (d.chip == Chip::BME280) {
        uint8_t h[7];
        if (!readRegs(w, d.addr, 0xA1, &k.H1, 1) || !readRegs(w, d.addr, 0xE1, h, 7)) return false;
        k.H2 = le16(h);
        k.H3 = h[2];
        k.H4 = (int16_t)((int8_t)h[3] * 16 | (h[4] & 0x0F));
        k.H5 = (int16_t)((int8_t)h[5] * 16 | (h[4] >> 4));
        k.H6 = (int8_t)h[6];
        writeReg(w, d.addr, 0xF2, 0x01); // humidity x1 (must precede ctrl_meas)
    }
    writeReg(w, d.addr, 0xF5, 0x10); // standby 0.5 ms, IIR filter x16: steady altitude
    writeReg(w, d.addr, 0xF4, 0x57); // temp x2, pressure x16, normal mode
    d.nextDue = millis() + 100;
    return true;
}

int bmeRead(TwoWire *w, WiredDevice &d, Reading *out) {
    uint8_t b[8];
    int n = d.chip == Chip::BME280 ? 8 : 6;
    if (!readRegs(w, d.addr, 0xF7, b, n)) return -1;
    int32_t adcP = (b[0] << 12) | (b[1] << 4) | (b[2] >> 4);
    int32_t adcT = (b[3] << 12) | (b[4] << 4) | (b[5] >> 4);
    if (adcT == 0x80000) return 0; // measurement skipped / not ready yet
    const auto &k = d.cal;

    // Floating-point compensation, datasheet section 8.1.
    double v1 = (adcT / 16384.0 - k.T1 / 1024.0) * k.T2;
    double v2 = (adcT / 131072.0 - k.T1 / 8192.0);
    v2 = v2 * v2 * k.T3;
    double tFine = v1 + v2;
    double t = tFine / 5120.0;

    v1 = tFine / 2.0 - 64000.0;
    v2 = v1 * v1 * k.P6 / 32768.0;
    v2 = v2 + v1 * k.P5 * 2.0;
    v2 = v2 / 4.0 + k.P4 * 65536.0;
    v1 = (k.P3 * v1 * v1 / 524288.0 + k.P2 * v1) / 524288.0;
    v1 = (1.0 + v1 / 32768.0) * k.P1;
    double p = 0;
    if (v1 != 0) {
        p = 1048576.0 - adcP;
        p = (p - v2 / 4096.0) * 6250.0 / v1;
        v1 = k.P9 * p * p / 2147483648.0;
        v2 = p * k.P8 / 32768.0;
        p = p + (v1 + v2 + k.P7) / 16.0;
    }

    int c = 0;
    out[c++] = {"Temp", CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, (float)t};
    if (p > 0) out[c++] = {"Pressure", CU_TYPE_PRESSURE, CU_UNIT_HPA, (float)(p / 100.0)};
    if (d.chip == Chip::BME280) {
        int32_t adcH = (b[6] << 8) | b[7];
        double h = tFine - 76800.0;
        h = (adcH - (k.H4 * 64.0 + k.H5 / 16384.0 * h)) *
            (k.H2 / 65536.0 * (1.0 + k.H6 / 67108864.0 * h * (1.0 + k.H3 / 67108864.0 * h)));
        h = h * (1.0 - k.H1 * h / 524288.0);
        out[c++] = {"Humidity", CU_TYPE_HUMIDITY, CU_UNIT_PERCENT, (float)constrain(h, 0.0, 100.0)};
    }
    return c;
}

// ---- SHT3x (single shot, high repeatability, no clock stretching) ------------

int shtRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    if (d.phase == 0) {
        if (!command16(w, d.addr, 0x2400)) return -1;
        d.phase = 1;
        d.nextDue = now + 20; // max 15.5 ms
        return 0;
    }
    d.phase = 0;
    d.nextDue = now + READ_INTERVAL_MS;
    uint8_t b[6];
    if (!readRaw(w, d.addr, b, 6)) return -1;
    if (!sensirionOk(b, 2)) return 0;
    float t = -45.0f + 175.0f * ((b[0] << 8) | b[1]) / 65535.0f;
    float h = 100.0f * ((b[3] << 8) | b[4]) / 65535.0f;
    out[0] = {"Temp", CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, t};
    out[1] = {"Humidity", CU_TYPE_HUMIDITY, CU_UNIT_PERCENT, constrain(h, 0.0f, 100.0f)};
    return 2;
}

// ---- AHT20 / AHT21 / AHT10 ----------------------------------------------------

int ahtRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    if (d.phase == 0) {
        const uint8_t trig[3] = {0xAC, 0x33, 0x00};
        if (!writeBytes(w, d.addr, trig, 3)) return -1;
        d.phase = 1;
        d.nextDue = now + 85; // 80 ms conversion
        return 0;
    }
    uint8_t b[6];
    if (!readRaw(w, d.addr, b, 6)) return -1;
    if (b[0] & 0x80) { // still busy
        d.nextDue = now + 20;
        return 0;
    }
    d.phase = 0;
    d.nextDue = now + READ_INTERVAL_MS;
    uint32_t rh = ((uint32_t)b[1] << 12) | (b[2] << 4) | (b[3] >> 4);
    uint32_t rt = ((uint32_t)(b[3] & 0x0F) << 16) | (b[4] << 8) | b[5];
    out[0] = {"Temp", CU_TYPE_TEMPERATURE, CU_UNIT_CELSIUS, rt * 200.0f / 1048576.0f - 50.0f};
    out[1] = {"Humidity", CU_TYPE_HUMIDITY, CU_UNIT_PERCENT, rh * 100.0f / 1048576.0f};
    return 2;
}

// ---- BH1750 (continuous high-res mode) ----------------------------------------

int bhRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    uint8_t b[2];
    if (!readRaw(w, d.addr, b, 2)) return -1;
    d.nextDue = now + READ_INTERVAL_MS;
    out[0] = {"Light", CU_TYPE_LIGHT, CU_UNIT_LUX, ((b[0] << 8) | b[1]) / 1.2f};
    return 1;
}

// ---- SGP30 (eCO2 / TVOC) -------------------------------------------------------

int sgpRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    if (d.phase == 0) {
        if (!command16(w, d.addr, 0x2008)) return -1; // measure_air_quality
        d.phase = 1;
        d.nextDue = now + 15; // max 12 ms
        return 0;
    }
    d.phase = 0;
    d.nextDue = now + SGP30_INTERVAL_MS - 15;
    uint8_t b[6];
    if (!readRaw(w, d.addr, b, 6)) return -1;
    if (!sensirionOk(b, 2)) return 0;
    out[0] = {"eCO2", CU_TYPE_CO2, CU_UNIT_PPM, (float)((b[0] << 8) | b[1])};
    out[1] = {"TVOC", CU_TYPE_TVOC, CU_UNIT_PPB, (float)((b[3] << 8) | b[4])};
    return 2;
}

// ---- MPU6050 family -------------------------------------------------------------

int mpuRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    uint8_t b[14];
    if (!readRegs(w, d.addr, 0x3B, b, 14)) return -1;
    d.nextDue = now + READ_INTERVAL_MS;
    auto s16 = [&](int i) { return (int16_t)((b[i] << 8) | b[i + 1]); };
    float ax = s16(0) / 16384.0f, ay = s16(2) / 16384.0f, az = s16(4) / 16384.0f; // +-2 g
    float gx = s16(8) / 131.0f, gy = s16(10) / 131.0f, gz = s16(12) / 131.0f;    // +-250 dps
    out[0] = {"Accel", CU_TYPE_ACCEL, CU_UNIT_G, sqrtf(ax * ax + ay * ay + az * az)};
    out[1] = {"Pitch", CU_TYPE_ANGLE, CU_UNIT_DEGREE, atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2958f};
    out[2] = {"Roll", CU_TYPE_ANGLE, CU_UNIT_DEGREE, atan2f(ay, az) * 57.2958f};
    out[3] = {"Gyro", CU_TYPE_GYRO, CU_UNIT_DEG_PER_S, sqrtf(gx * gx + gy * gy + gz * gz)};
    return 4;
}

// ---- INA219 (power-on defaults: 32 V, +-320 mV shunt; assumes a 0.1 ohm shunt) ---

int inaRead(TwoWire *w, WiredDevice &d, Reading *out, uint32_t now) {
    uint8_t s[2], v[2];
    if (!readRegs(w, d.addr, 0x01, s, 2) || !readRegs(w, d.addr, 0x02, v, 2)) return -1;
    d.nextDue = now + READ_INTERVAL_MS;
    float shuntMv = (int16_t)((s[0] << 8) | s[1]) * 0.01f;
    float busV = (((v[0] << 8) | v[1]) >> 3) * 0.004f;
    float mA = shuntMv / 0.1f;
    out[0] = {"Voltage", CU_TYPE_VOLTAGE, CU_UNIT_VOLT, busV};
    out[1] = {"Current", CU_TYPE_CURRENT, CU_UNIT_MILLIAMP, mA};
    out[2] = {"Power", CU_TYPE_POWER, CU_UNIT_MILLIWATT, busV * mA};
    return 3;
}

} // namespace

bool identify(TwoWire *w, uint8_t addr, WiredDevice &d) {
    d = WiredDevice();
    d.addr = addr;
    d.chip = Chip::Unknown;
    const char *guess = guessName(addr);
    setName(d, guess ? guess : "?");
    uint8_t b[9];

    if (addr == 0x24) {
        d.chip = Chip::PN532;
        setName(d, "PN532 NFC");
        return true;
    }
    if (addr == 0x76 || addr == 0x77) {
        if (!readRegs(w, addr, 0xD0, b, 1)) return false;
        if (b[0] == 0x60) d.chip = Chip::BME280, setName(d, "BME280");
        else if (b[0] >= 0x56 && b[0] <= 0x58) d.chip = Chip::BMP280, setName(d, "BMP280");
        else if (b[0] == 0x61) d.chip = Chip::Unsupported, setName(d, "BME680");
        else if (b[0] == 0x55) d.chip = Chip::Unsupported, setName(d, "BMP180");
        if (readable(d.chip) && !bmeSetup(w, d)) d.chip = Chip::Unknown;
        return true;
    }
    if (addr == 0x44 || addr == 0x45) {
        // Read status register: a correct CRC means a Sensirion SHT3x.
        if (command16(w, addr, 0xF32D)) {
            delay(1);
            if (readRaw(w, addr, b, 3) && sensirionOk(b, 1)) d.chip = Chip::SHT3x, setName(d, "SHT3x");
        }
        return true;
    }
    if (addr == 0x38) {
        if (!readRaw(w, addr, b, 1)) return false;
        if (!(b[0] & 0x08)) { // not calibrated: send the init command
            const uint8_t init[3] = {0xBE, 0x08, 0x00};
            writeBytes(w, addr, init, 3);
            delay(10);
        }
        d.chip = Chip::AHT20;
        setName(d, "AHT2x");
        return true;
    }
    if (addr == 0x23 || addr == 0x5C) {
        // No ID register; power on + continuous high-res, and see if it ACKs.
        uint8_t on = 0x01, mode = 0x10;
        if (writeBytes(w, addr, &on, 1) && writeBytes(w, addr, &mode, 1)) {
            d.chip = Chip::BH1750;
            setName(d, "BH1750");
            d.nextDue = millis() + 200; // first conversion ~120-180 ms
        }
        return true;
    }
    if (addr == 0x58) {
        if (command16(w, addr, 0x3682)) { // get_serial_id
            delay(1);
            if (readRaw(w, addr, b, 9) && sensirionOk(b, 3)) {
                d.chip = Chip::SGP30;
                setName(d, "SGP30");
                command16(w, addr, 0x2003); // init_air_quality: 15 s warm-up starts now
                d.nextDue = millis() + SGP30_INTERVAL_MS;
            }
        }
        return true;
    }
    if (addr == 0x68 || addr == 0x69) {
        if (!readRegs(w, addr, 0x75, b, 1)) return false;
        const char *n = nullptr;
        switch (b[0]) {
            case 0x68: n = "MPU6050"; break;
            case 0x70: n = "MPU6500"; break;
            case 0x71: n = "MPU9250"; break;
            case 0x73: n = "MPU9255"; break;
        }
        if (n) {
            d.chip = Chip::MPU6050;
            setName(d, n);
            writeReg(w, addr, 0x6B, 0x01); // wake, PLL on gyro X
            d.nextDue = millis() + 50;
        }
        return true;
    }
    if (addr >= 0x40 && addr <= 0x4F) {
        // INA219 has no ID register; its config register resets to 0x399F.
        if (readRegs(w, addr, 0x00, b, 2) && ((b[0] << 8) | b[1]) == 0x399F)
            d.chip = Chip::INA219, setName(d, "INA219");
        return true;
    }
    return true;
}

const char *guessName(uint8_t addr) {
    struct Guess {
        uint8_t lo, hi;
        const char *name;
    };
    static const Guess GUESSES[] = {
        {0x0D, 0x0D, "QMC5883L?"},  {0x10, 0x10, "VEML7700?"}, {0x1E, 0x1E, "HMC5883L?"},
        {0x20, 0x27, "IO expander?"}, {0x29, 0x29, "VL53L0X?"},  {0x36, 0x36, "MAX17048?"},
        {0x39, 0x39, "APDS9960?"},  {0x3C, 0x3D, "OLED?"},      {0x40, 0x40, "HTU21D?"},
        {0x48, 0x4B, "ADS1115?"},   {0x50, 0x57, "EEPROM?"},    {0x53, 0x53, "ADXL345?"},
        {0x5A, 0x5B, "CCS811?"},    {0x61, 0x61, "SCD30?"},     {0x62, 0x62, "SCD4x?"},
        {0x68, 0x68, "DS3231 RTC?"}, {0x70, 0x70, "TCA9548A?"},
    };
    for (const auto &g : GUESSES)
        if (addr >= g.lo && addr <= g.hi) return g.name;
    return nullptr;
}

int poll(TwoWire *w, WiredDevice &d, Reading out[MAX_READINGS], uint32_t now) {
    if (!readable(d.chip) || (int32_t)(now - d.nextDue) < 0) return 0;
    int n = -1;
    switch (d.chip) {
        case Chip::BME280:
        case Chip::BMP280:
            n = bmeRead(w, d, out);
            d.nextDue = now + READ_INTERVAL_MS;
            break;
        case Chip::SHT3x: n = shtRead(w, d, out, now); break;
        case Chip::AHT20: n = ahtRead(w, d, out, now); break;
        case Chip::BH1750: n = bhRead(w, d, out, now); break;
        case Chip::SGP30: n = sgpRead(w, d, out, now); break;
        case Chip::MPU6050: n = mpuRead(w, d, out, now); break;
        case Chip::INA219: n = inaRead(w, d, out, now); break;
        default: break;
    }
    if (n < 0) {
        d.errors++;
        d.phase = 0;
        d.nextDue = now + READ_INTERVAL_MS;
    } else if (n > 0) d.errors = 0;
    return n;
}

} // namespace sensors
