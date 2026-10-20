#include "hooks.h"

#include "net/net.h"
#include "remote/remote.h"
#include "core/display.h"

bool cyberAppsHandleLine(const String &line, bool &result) {
    if (remote::handleLine(line)) {
        result = true;
        return true;
    }
    return false;
}

void cyberAppsBoot() { remote::boot(); }

void cyberAppsWifiJoined() {
    String detail;
    net::Internet i = net::checkInternet(detail);
    if (i == net::NET_ONLINE) return;
    Serial.println("[net] " + net::internetText(i) + (detail.length() ? " (" + detail + ")" : String("")));
    // A sign-in page is worth a tap to dismiss: nothing online will work on it.
    displayWarning(i == net::NET_CAPTIVE ? "Wi-Fi needs a sign-in page" : "Joined, but no internet", i == net::NET_CAPTIVE);
}
