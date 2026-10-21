#include "hat_probe.h"

#include "../hardware/hardware.h"

#include "core/bus_HAL.h"
#include <SPI.h>
#include <globals.h>

namespace {

constexpr uint8_t PN532_I2C_ADDR = 0x24;

// CC1101 status registers are read with the burst bit set (0xC0 | addr).
constexpr uint8_t CC1101_READ_PARTNUM = 0xF0;
constexpr uint8_t CC1101_READ_VERSION = 0xF1;

// nRF24 R_REGISTER is 0x00 | addr; SETUP_AW (0x03) resets to 0x03.
constexpr uint8_t NRF24_READ_SETUP_AW = 0x03;

const SPISettings PROBE_SPI(1000000, MSBFIRST, SPI_MODE0);

bool isKnownCcVersion(uint8_t v) {
    // Known CC1101 VERSION values across silicon revisions and clones.
    return v == 0x04 || v == 0x07 || v == 0x08 || v == 0x14 || v == 0x17 || v == 0x18;
}

// A chip left mid-byte (e.g. by the SPI probe toggling 27/22 under it) can hold SDA
// low, and then nothing on the bus answers until power-off. Standard recovery: clock
// SCL up to 9 times until SDA is released, then a STOP. Only acts if SDA is held low.
void recoverI2C() {
    int sda = bruceConfigPins.i2c_bus.sda, scl = bruceConfigPins.i2c_bus.scl;
    if (sda < 0 || scl < 0) return;
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(10);
    if (digitalRead(sda)) return;
    pinMode(scl, OUTPUT_OPEN_DRAIN);
    for (int i = 0; i < 9 && !digitalRead(sda); i++) {
        digitalWrite(scl, LOW);
        delayMicroseconds(5);
        digitalWrite(scl, HIGH);
        delayMicroseconds(5);
    }
    pinMode(sda, OUTPUT_OPEN_DRAIN); // STOP: SDA low -> high while SCL is high
    digitalWrite(sda, LOW);
    delayMicroseconds(5);
    digitalWrite(sda, HIGH);
    delayMicroseconds(5);
    pinMode(sda, INPUT);
    pinMode(scl, INPUT);
}

// Both lines idle high once the pull-ups have a moment. One held low means a chip
// is stretching the clock or stuck mid-byte, and every address would time out.
bool linesIdle() {
    int sda = bruceConfigPins.i2c_bus.sda, scl = bruceConfigPins.i2c_bus.scl;
    if (sda < 0 || scl < 0) return true;
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);
    delayMicroseconds(20);
    bool idle = digitalRead(sda) && digitalRead(scl);
    pinMode(sda, INPUT);
    pinMode(scl, INPUT);
    return idle;
}

// Per-address timeout, and the most one scan may take. An absent address NACKs in
// well under a millisecond; Wire's own 50 ms timeout times 112 addresses, six scans
// at boot, kept the serial task from starting until after the website gave up.
constexpr uint16_t I2C_PROBE_TIMEOUT_MS = 10;
constexpr uint32_t I2C_SCAN_BUDGET_MS = 250;
constexpr uint8_t WIRE_TIMEOUT = 5; // endTransmission(): ESP_ERR_TIMEOUT

void probeI2C(HatProbeResult &r) {
    recoverI2C();
    if (!linesIdle()) {
        r.i2cStuck = true;
        return;
    }
    TwoWire *wire = acquireI2CBus();
    uint16_t prevTimeout = wire->getTimeOut();
    wire->setTimeOut(I2C_PROBE_TIMEOUT_MS);
    auto ask = [wire](uint8_t addr) {
        wire->beginTransmission(addr);
        return wire->endTransmission();
    };

    // The PN532 first, so a slow bus still finds the reader before the budget runs out.
    uint8_t err = ask(PN532_I2C_ADDR);
    if (err != 0 && err != WIRE_TIMEOUT) {
        // A sleeping PN532 can NACK the address byte that wakes it: ask again.
        delay(2);
        err = ask(PN532_I2C_ADDR);
    }
    if (err == 0) r.pn532 = true;
    else if (err == WIRE_TIMEOUT) r.i2cStuck = true;

    uint32_t started = millis();
    for (uint8_t addr = 0x08; addr < 0x78 && !r.i2cStuck; addr++) {
        if (addr == PN532_I2C_ADDR) continue;
        if (millis() - started > I2C_SCAN_BUDGET_MS) {
            r.i2cStuck = true;
            break;
        }
        err = ask(addr);
        if (err == WIRE_TIMEOUT) r.i2cStuck = true;
        else if (err == 0) r.i2cOther.push_back(addr);
    }
    wire->setTimeOut(prevTimeout);
    releaseI2CBus();
}

// Raw two-byte exchange with CS held low. Returns {first, second} response bytes.
void spiExchange(SPIClass *spi, gpio_num_t cs, uint8_t cmd, uint8_t &first, uint8_t &second) {
    digitalWrite(cs, LOW);
    delayMicroseconds(50); // CC1101 pulls MISO low when its crystal is ready
    spi->beginTransaction(PROBE_SPI);
    first = spi->transfer(cmd);
    second = spi->transfer(0x00);
    spi->endTransaction();
    digitalWrite(cs, HIGH);
    delayMicroseconds(20);
}

void probeSPI(HatProbeResult &r) {
    const auto &cc = bruceConfigPins.CC1101_bus;
    const auto &nrf = bruceConfigPins.NRF24_bus;

    // Shared lines: keep both chip selects idle and hold the nRF24 CE / CC1101
    // GDO0 / IR+433 TX line LOW so nothing transmits while we poke the bus.
    pinMode(cc.cs, OUTPUT);
    digitalWrite(cc.cs, HIGH);
    if (nrf.cs != cc.cs) {
        pinMode(nrf.cs, OUTPUT);
        digitalWrite(nrf.cs, HIGH);
    }
    if (nrf.io0 != GPIO_NUM_NC) {
        pinMode(nrf.io0, OUTPUT);
        digitalWrite(nrf.io0, LOW);
    }
    // Keep the SD card deselected; it shares SCK/MISO/MOSI.
    if (bruceConfigPins.SDCARD_bus.cs != GPIO_NUM_NC) {
        pinMode(bruceConfigPins.SDCARD_bus.cs, OUTPUT);
        digitalWrite(bruceConfigPins.SDCARD_bus.cs, HIGH);
    }
    delay(2);

    SPIClass *spi = acquireSPIBus(cc.sck, cc.miso, cc.mosi);
    if (!spi) {
        spi = &SPI;
        spi->begin(cc.sck, cc.miso, cc.mosi, -1);
    }

    // CC1101 first: its PARTNUM/VERSION reads are read-only. On an nRF24 these
    // command bytes are undefined and just return STATUS, so they can't match.
    uint8_t dummy;
    spiExchange(spi, cc.cs, CC1101_READ_PARTNUM, dummy, r.ccPartnum);
    spiExchange(spi, cc.cs, CC1101_READ_VERSION, dummy, r.ccVersion);
    r.cc1101 = (r.ccPartnum == 0x00) && isKnownCcVersion(r.ccVersion);

    // Only probe the nRF24 if no CC1101 answered: 0x03 is a register *write*
    // on a CC1101 (FIFOTHR), and we never write to a chip we didn't mean to.
    if (!r.cc1101) {
        spiExchange(spi, nrf.cs, NRF24_READ_SETUP_AW, r.nrfStatus, r.nrfSetupAw);
        r.nrf24 = (r.nrfStatus != 0x00) && ((r.nrfStatus & 0x80) == 0) && r.nrfSetupAw >= 1 &&
                  r.nrfSetupAw <= 3;
    }

    // Leave the shared pins as inputs so the next app starts clean.
    pinMode(cc.cs, INPUT);
    if (nrf.cs != cc.cs) pinMode(nrf.cs, INPUT);
    if (nrf.io0 != GPIO_NUM_NC) pinMode(nrf.io0, INPUT);
}

void probeRxLine(HatProbeResult &r) {
    int pin = bruceConfigPins.rfRx;
    if (pin < 0) return;
    pinMode(pin, INPUT_PULLDOWN);
    delay(5);

    const int samples = 6000; // 300 ms at 50 us
    int high = 0, transitions = 0;
    int last = digitalRead(pin);
    for (int i = 0; i < samples; i++) {
        int v = digitalRead(pin);
        if (v) high++;
        if (v != last) transitions++;
        last = v;
        delayMicroseconds(50);
    }
    pinMode(pin, INPUT);

    r.rxChecked = true;
    r.rxTransitions = transitions;
    r.rxHighPercent = (high * 100) / samples;
    r.ookLikely = transitions > 20;
    r.irLikely = !r.ookLikely && r.rxHighPercent > 95;
}

String hex2(uint8_t v) {
    char buf[5];
    snprintf(buf, sizeof(buf), "0x%02X", v);
    return String(buf);
}

} // namespace

int HatProbeResult::liveCount() const {
    return int(pn532) + int(cc1101) + int(nrf24) + int(irLikely) + int(ookLikely);
}

String HatProbeResult::summary() const {
    std::vector<String> live;
    if (cc1101) live.push_back("CC1101 (DIP 1)");
    if (nrf24) live.push_back("nRF24 (DIP 2)");
    if (pn532) live.push_back("PN532 (DIP 3)");
    if (irLikely) live.push_back("IR, likely (DIP 4)");
    if (ookLikely) live.push_back("433 OOK, likely (DIP 5)");
    if (live.empty()) return "none";
    String s = live[0];
    for (size_t i = 1; i < live.size(); i++) s += ", " + live[i];
    return s;
}

String HatProbeResult::advice() const {
    if (i2cStuck) return "The card reader is holding the bus. Unplug for 10 s, DIP 3 only.";
    int n = liveCount();
    if (n == 0) return "Nothing answered. Turn ON one of DIP 1-5.";
    if (n > 1) return "More than one module is on. Only one of DIP 1-5 at a time.";
    if (irLikely || ookLikely)
        return "IR/433 have no bus to ask; this is a guess from the RX pin.";
    return "Ready.";
}

namespace {
SemaphoreHandle_t hatBus() {
    static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    return m;
}
} // namespace

bool hatI2CRecover() {
    recoverI2C();
    return linesIdle();
}

bool hatBusTake(uint32_t waitMs) { return xSemaphoreTakeRecursive(hatBus(), pdMS_TO_TICKS(waitMs)) == pdTRUE; }

void hatBusGive() { xSemaphoreGiveRecursive(hatBus()); }

static HatProbeResult lastResult;
static uint32_t lastAt = 0;
static bool haveLast = false;

HatProbeResult probeHat() { return probeHat(true); }

HatProbeResult probeHat(bool allowSpi) {
    if (!hatBusTake(0)) return lastResult; // the reader is in use elsewhere: report what we last saw
    HatProbeResult r;
    probeI2C(r);
    if (!r.pn532 && !r.i2cStuck) {
        // Ask the I2C bus once more before driving 22/27 as SPI: a PN532 that was
        // asleep answers the second time, and SPI traffic under a half-awake one
        // can wedge it until the next power cycle.
        delay(20);
        r.i2cOther.clear();
        probeI2C(r);
    }
    // The SPI radios sit on the same two pins as I2C. If the PN532 answered, or
    // something is holding the lines, driving them as SPI would fight it: stop here.
    if (r.i2cStuck) Serial.println("[hat] I2C bus held low or timing out; skipped the rest of the probe");
    if (allowSpi && !r.pn532 && !r.i2cStuck) probeSPI(r);
    if (allowSpi && !r.i2cStuck && r.liveCount() == 0) probeRxLine(r);
    lastResult = r;
    lastAt = millis();
    haveLast = true;
    hatBusGive();
    hw::ingestProbe(r); // keeps the Library / main menu in step with what is plugged in
    return r;
}

bool lastHatProbe(HatProbeResult &out, uint32_t &atMs) {
    if (!haveLast) return false;
    out = lastResult;
    atMs = lastAt;
    return true;
}

std::vector<String> hatReportLines(const HatProbeResult &r) {
    std::vector<String> l;
    l.push_back("Live: " + r.summary());
    l.push_back(r.advice());
    l.push_back("");
    if (r.pn532) {
        l.push_back("CC1101  skipped (PN532 owns pins 22/27)");
        l.push_back("nRF24   skipped (PN532 owns pins 22/27)");
    } else {
        l.push_back(String("CC1101  ") + (r.cc1101 ? "yes" : "no") + "  part " + hex2(r.ccPartnum) +
                    " ver " + hex2(r.ccVersion));
        if (r.cc1101) l.push_back("nRF24   skipped (CC1101 owns the bus)");
        else
            l.push_back(String("nRF24   ") + (r.nrf24 ? "yes" : "no") + "  status " + hex2(r.nrfStatus) +
                        " aw " + hex2(r.nrfSetupAw));
    }
    l.push_back(String("PN532   ") + (r.pn532 ? "yes" : "no") + "  (I2C 0x24)");
    if (r.i2cStuck) l.push_back("I2C bus stuck: scan stopped early");
    if (r.rxChecked)
        l.push_back("RX pin  " + String(r.rxTransitions) + " edges, " + String(r.rxHighPercent) + "% high");
    else l.push_back("RX pin  not checked (a module already answered)");
    String other = "I2C other: ";
    if (r.i2cOther.empty()) other += "none";
    for (size_t i = 0; i < r.i2cOther.size(); i++) other += (i ? ", " : "") + hex2(r.i2cOther[i]);
    l.push_back(other);
    return l;
}
