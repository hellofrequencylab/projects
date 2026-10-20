// Hat probe: works out which NM-RF-HAT module is powered (DIP 1-5) by asking
// each one directly. Every module shares pins 22/27, so the probe order matters:
// I2C first (read-only address ACKs), then SPI register reads, then a passive
// listen on the shared RX pin for the modules that have no bus at all.
#pragma once

#include <Arduino.h>
#include <vector>

struct HatProbeResult {
    // PN532 (DIP 3) and anything else on the Grove I2C bus (SDA 27 / SCL 22).
    bool pn532 = false;
    std::vector<uint8_t> i2cOther; // addresses that ACKed, excluding the PN532

    // CC1101 (DIP 1): PARTNUM / VERSION status registers.
    bool cc1101 = false;
    uint8_t ccPartnum = 0xFF;
    uint8_t ccVersion = 0xFF;

    // nRF24 (DIP 2): STATUS byte and SETUP_AW register.
    bool nrf24 = false;
    uint8_t nrfStatus = 0xFF;
    uint8_t nrfSetupAw = 0xFF;

    // Passive listen on the shared RX pin (27) with a pull-down.
    // An IR receiver idles HIGH; a 433 MHz OOK receiver outputs noise.
    bool rxChecked = false;
    int rxTransitions = 0;
    int rxHighPercent = 0;
    bool irLikely = false;
    bool ookLikely = false;

    int liveCount() const;
    String summary() const; // one line, e.g. "CC1101 (DIP 1)"
    String advice() const;  // what to do next
};

HatProbeResult probeHat();
// allowSpi = false: I2C scan only, never drives 22/27 as SPI or reads the RX line
// (the boot probe's early tries, while a PN532 may still be waking).
HatProbeResult probeHat(bool allowSpi);

// The most recent probeHat() result and its millis() timestamp; false if none yet.
// Lets status screens report the hat without driving the shared pins again.
bool lastHatProbe(HatProbeResult &out, uint32_t &atMs);

// Human-readable report, one entry per line, shared by the screen and the CLI.
std::vector<String> hatReportLines(const HatProbeResult &r);
