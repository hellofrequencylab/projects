#include "hardware.h"

#include "../hat/hat_probe.h"
#include "../lightshow/LightShowMenu.h"
#include <Preferences.h>

namespace hw {
namespace {

const PartInfo PARTS[HW_PART_COUNT] = {
    {"pn532",    "NFC reader (PN532)",       "Hat DIP 3 ON, 1, 2, 4, 5 OFF",                  true,  3},
    {"cc1101",   "Sub-GHz radio (CC1101)",   "Hat DIP 1 ON, 2 to 5 OFF",                      true,  1},
    {"nrf24",    "2.4 GHz radio (nRF24)",    "Hat DIP 2 ON, 1, 3, 4, 5 OFF",                  true,  2},
    {"ir",       "IR transmitter/receiver",  "Hat DIP 4 ON, 1 to 3 and 5 OFF",                true,  4},
    {"ook433",   "433 MHz module",           "Hat DIP 5 ON, 1 to 4 OFF",                      true,  5},
    {"i2c",      "I2C sensor",               "Grove port (SDA 27, SCL 22). DIP 1 to 5 OFF, or only 3", true, 0},
    {"node",     "Wireless sensor node",     "ESP-NOW node; open Sensors to listen for it",   true,  0},
    {"speaker",  "Speaker",                  "Small speaker on the CYD SPEAK plug",           false, 0},
    {"ledpanel", "8x8 LED panel (WS2812)",   "Data to pin 27, own 5 V supply. DIP 1 to 5 OFF", false, 0},
    {"gps",      "GPS module",               "Serial GPS. Shares the USB serial pins",        false, 0},
    {"lora",     "LoRa module",              "SPI LoRa radio on the hat pins",                false, 0},
    {"ethernet", "Ethernet (W5500)",         "SPI W5500 on the hat pins",                     false, 0},
    {"fm",       "FM transmitter (Si4713)",  "I2C Si4713 on the Grove port",                  false, 0},
};

// Apps that need hardware. Anything not listed is always in the main menu.
const AppNeed NEEDS[] = {
    {"Frequency",  uint16_t(1u << HW_PN532)                          },
    {"RFID",       uint16_t(1u << HW_PN532)                          },
    {"RF",         uint16_t((1u << HW_CC1101) | (1u << HW_OOK433))      },
    {"NRF24",      uint16_t(1u << HW_NRF24)                          },
    {"IR",         uint16_t(1u << HW_IR)                             },
    {"Sensors",    uint16_t((1u << HW_I2C_SENSOR) | (1u << HW_SENSOR_NODE))},
    {"Sound",      uint16_t(1u << HW_SPEAKER)                        },
    {"Light Show", uint16_t(1u << HW_LED_PANEL)                      },
    {"GPS",        uint16_t(1u << HW_GPS)                            },
    {"LoRa",       uint16_t(1u << HW_LORA)                           },
    {"Ethernet",   uint16_t(1u << HW_ETHERNET)                       },
    {"FM",         uint16_t(1u << HW_FM)                             },
};

constexpr const char *NVS_NS = "cyberhw";

uint32_t detectedMask = 0;
uint32_t markedMask = 0;
uint32_t liveMask = 0;
volatile uint32_t pendingMask = 0; // set from callbacks, folded in by flush()
bool loaded = false;

void load() {
    if (loaded) return;
    Preferences p;
    if (p.begin(NVS_NS, true)) {
        detectedMask = p.getUInt("seen", 0);
        markedMask = p.getUInt("marked", 0);
        p.end();
    }
    loaded = true;
}

void save() {
    Preferences p;
    if (!p.begin(NVS_NS, false)) return;
    p.putUInt("seen", detectedMask);
    p.putUInt("marked", markedMask);
    p.end();
}

// Folds callback-side detections in; main/CLI task only (NVS write).
void flush() {
    load();
    uint32_t pend = pendingMask;
    if (!pend) return;
    pendingMask = 0;
    if ((detectedMask | pend) != detectedMask) {
        detectedMask |= pend;
        save();
    }
}

} // namespace

const PartInfo &info(Part p) { return PARTS[p < HW_PART_COUNT ? p : 0]; }

bool partFromId(const String &id, Part &out) {
    for (int i = 0; i < HW_PART_COUNT; i++)
        if (id.equalsIgnoreCase(PARTS[i].id)) {
            out = Part(i);
            return true;
        }
    return false;
}

bool detected(Part p) {
    flush();
    return detectedMask & (1u << p);
}
bool marked(Part p) {
    load();
    return markedMask & (1u << p);
}
bool liveNow(Part p) { return liveMask & (1u << p); }
bool present(Part p) { return detected(p) || marked(p) || liveNow(p); }

void setMarked(Part p, bool on) {
    load();
    uint32_t m = on ? (markedMask | (1u << p)) : (markedMask & ~(1u << p));
    if (m == markedMask) return;
    markedMask = m;
    save();
}

void forget(Part p) {
    flush();
    detectedMask &= ~(1u << p);
    markedMask &= ~(1u << p);
    liveMask &= ~(1u << p);
    save();
}

void forgetAll() {
    load();
    pendingMask = 0;
    detectedMask = markedMask = liveMask = 0;
    save();
}

void noteDetected(Part p) { pendingMask |= (1u << p); }

void ingestProbe(const HatProbeResult &r) {
    uint32_t live = 0;
    if (r.pn532) live |= 1u << HW_PN532;
    if (r.cc1101) live |= 1u << HW_CC1101;
    if (r.nrf24) live |= 1u << HW_NRF24;
    // IR/433 come from a noise heuristic on the RX pin: trust it only when it is
    // the sole thing the probe found, so stray noise can't add both.
    if (!r.pn532 && !r.cc1101 && !r.nrf24) {
        if (r.irLikely) live |= 1u << HW_IR;
        if (r.ookLikely) live |= 1u << HW_OOK433;
    }
    // Other I2C addresses are only meaningful while the bus isn't driven as SPI.
    if (!r.cc1101 && !r.nrf24 && !r.i2cOther.empty()) live |= 1u << HW_I2C_SENSOR;

    liveMask = live;
    pendingMask |= live;
}

bool rescan() {
    if (lightshowRunning()) return false;
    probeHat(); // calls ingestProbe()
    flush();
    return true;
}

const AppNeed *needFor(const String &menuName) {
    for (const AppNeed &n : NEEDS)
        if (menuName == n.menuName) return &n;
    return nullptr;
}

bool appActive(const String &menuName) {
    const AppNeed *n = needFor(menuName);
    if (!n) return true;
    for (int i = 0; i < HW_PART_COUNT; i++)
        if ((n->anyOf & (1u << i)) && present(Part(i))) return true;
    return false;
}

bool appNeedsHatModule(const String &menuName) {
    const AppNeed *n = needFor(menuName);
    if (!n) return false;
    for (int i = 0; i < HW_PART_COUNT; i++)
        if ((n->anyOf & (1u << i)) && PARTS[i].dip == 0) return false;
    return true;
}

bool appReadyNow(const String &menuName) {
    const AppNeed *n = needFor(menuName);
    if (!n) return true;
    for (int i = 0; i < HW_PART_COUNT; i++)
        if ((n->anyOf & (1u << i)) && (liveNow(Part(i)) || marked(Part(i)))) return true;
    return false;
}

void boot() {
    load();
    rescan();
}

} // namespace hw
