// Hardware registry: which add-on parts this unit has, and which apps need them.
// The main menu shows an app only when one of the parts it needs is present; the
// rest wait in the Library (LibraryMenu) until their hardware turns up.
//
// "Present" is sticky: a part counts once it has been detected (or marked by
// hand) and stays until forgotten, because the hat's DIP switch allows only one
// module at a time and the menu shouldn't reshuffle every time it is flipped.
// Parts the probe can't see (speaker, LED panel, GPS...) are marked by hand.
#pragma once

#include <Arduino.h>

struct HatProbeResult;

namespace hw {

enum Part : uint8_t {
    HW_PN532,       // NFC reader, hat DIP 3
    HW_CC1101,      // sub-GHz radio, hat DIP 1
    HW_NRF24,       // 2.4 GHz radio, hat DIP 2
    HW_IR,          // IR TX/RX, hat DIP 4 (probe says "likely")
    HW_OOK433,      // 433 MHz OOK module, hat DIP 5 (probe says "likely")
    HW_I2C_SENSOR,  // any I2C device on the Grove port other than the PN532
    HW_SENSOR_NODE, // ESP-NOW sensor node (seen by the Sensors app)
    HW_SPEAKER,     // speaker on the CYD's SPEAK plug
    HW_LED_PANEL,   // 8x8 WS2812 panel
    HW_GPS,         // serial GPS module
    HW_LORA,        // LoRa module
    HW_ETHERNET,    // W5500 Ethernet module
    HW_FM,          // Si4713 FM transmitter
    HW_PART_COUNT
};

struct PartInfo {
    const char *id;     // CLI / remote id, e.g. "pn532"
    const char *name;   // shown on screen
    const char *howTo;  // one line: switch position or wiring
    bool autoDetect;    // the hat probe (or an app) can see it
    uint8_t dip;        // hat DIP switch (1-5) that powers it, 0 = not a hat module
};

const PartInfo &info(Part p);
bool partFromId(const String &id, Part &out);

bool present(Part p); // detected at some point, or marked by hand
bool detected(Part p);
bool marked(Part p);
bool liveNow(Part p); // answered the most recent probe

void setMarked(Part p, bool on);
void forget(Part p); // clears detected + marked
void forgetAll();

// Safe from any task, including ESP-NOW / Wi-Fi callbacks: sets a RAM flag, saved later.
void noteDetected(Part p);

// Folds a probe result in. probeHat() calls this itself.
void ingestProbe(const HatProbeResult &r);

// Probes the hat now (~0.4 s; drives pins 22/27). Returns false, without probing,
// if something is using those pins (e.g. Light Show).
bool rescan(bool allowSpi = true); // false: I2C only (see probeHat(bool))

// --- apps --------------------------------------------------------------------

struct AppNeed {
    const char *menuName; // MenuItemInterface name
    uint16_t anyOf;       // bit per Part: any one present makes the app active
    bool pinned;          // always in the main menu; the hardware card shows on open
};

const AppNeed *needFor(const String &menuName); // nullptr = no rule, always active
bool appActive(const String &menuName);
// True if every part the app can use is a hat module (picked with the DIP switch),
// so opening it with the wrong switch on deserves the hardware card.
bool appNeedsHatModule(const String &menuName);
// True if any part the app can use answered the latest probe (or was marked by hand).
bool appReadyNow(const String &menuName);
inline uint16_t partBit(Part p) { return uint16_t(1u) << p; }

void boot(); // loads saved state and runs the first probe; once, from setup()

} // namespace hw
