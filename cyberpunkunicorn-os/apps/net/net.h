// Net: Wi-Fi helpers shared by our apps and the console protocol.
//
// - checkInternet(): is a joined network really online, or is it a guest network
//   with a splash page (captive portal) the board can't click through?
// - joinNetwork(): joins without drawing, so the CLI task (console `wifi.join`)
//   can call it. Bruce's own join draws on the screen and must stay on the UI task.
//
// Console commands (apps/remote/PROTOCOL.md): wifi.status, wifi.scan, wifi.join,
// wifi.forget, in net_remote.cpp. Serial: `netcheck`.
#pragma once

#include <Arduino.h>

class SimpleCLI;

namespace net {

enum Internet {
    NET_NO_WIFI,     // not joined to a network
    NET_ONLINE,      // the probe got its 204
    NET_CAPTIVE,     // something answered for the probe: a sign-in page
    NET_NO_INTERNET, // joined, but nothing answered
};

// Plain HTTP GET of a 204 probe, redirects not followed. `detail` gets the
// portal's address (Location header) when there is one, or the error.
Internet checkInternet(String &detail, uint32_t timeoutMs = 5000);
const char *internetName(Internet i); // "online", "captive", "no_internet", "no_wifi"
// One line for a person: "Online", "This Wi-Fi needs a sign-in page...".
String internetText(Internet i);

struct JoinResult {
    bool ok = false;
    String reason; // when !ok: "not_found", "wrong_password", "timeout", "low_memory", "busy"
    String ip;
};
// Joins `ssid` (empty password = open network), waiting up to timeoutMs. Turns the
// Bluetooth remote off first if Wi-Fi wouldn't fit beside it. Saves the network in
// Bruce's list on success when `save`. Never draws.
JoinResult joinNetwork(const String &ssid, const String &password, bool save, uint32_t timeoutMs = 20000);

} // namespace net

// Serial: `netcheck` prints whether the joined network reaches the internet.
void createNetCommand(SimpleCLI *cli);
