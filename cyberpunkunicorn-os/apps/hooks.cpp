#include "hooks.h"

#include "hardware/hardware.h"
#include "net/net.h"
#include "remote/remote.h"
#include "core/display.h"
#include <Preferences.h>
#include <globals.h>

bool cyberAppsHandleLine(const String &line, bool &result) {
    if (remote::handleLine(line)) {
        result = true;
        return true;
    }
    return false;
}

// boot.conf NEVER_DIM=1: turn the screen dimmer off once per unit. Bruce keeps
// dimmerSet in its saved config, so a compile-time default wouldn't reach a unit
// that already has one; the NVS flag means a dim time picked later in Config stays.
static void neverDimOnce() {
#ifdef CU_NEVER_DIM
    Preferences p;
    if (!p.begin("cyberhw", false)) return;
    if (!p.getBool("nodim", false)) {
        bruceConfig.setDimmer(0); // 0 = never dim or sleep
        p.putBool("nodim", true);
    }
    p.end();
#endif
}

void cyberAppsBoot() {
    neverDimOnce();
    hw::boot(); // first hat probe: sorts apps into the main menu or the Library
    remote::boot();
}

void cyberAppsWifiJoined() {
    String detail;
    net::Internet i = net::checkInternet(detail);
    if (i == net::NET_ONLINE) return;
    Serial.println("[net] " + net::internetText(i) + (detail.length() ? " (" + detail + ")" : String("")));
    // A sign-in page is worth a tap to dismiss: nothing online will work on it.
    displayWarning(i == net::NET_CAPTIVE ? "Wi-Fi needs a sign-in page" : "Joined, but no internet", i == net::NET_CAPTIVE);
}
