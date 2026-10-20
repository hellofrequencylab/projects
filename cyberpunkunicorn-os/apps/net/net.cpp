#include "net.h"

#include "../remote/remote.h"
#include "core/radio_mem.h"
#include "core/settings.h"
#include "modules/ble/ble_common.h"
#include <HTTPClient.h>
#include <SimpleCLI.h>
#include <WiFi.h>
#include <globals.h>

namespace net {

// Google's probe: answers 204 with no body on a real internet connection. A
// captive portal intercepts it and answers 200 (its page) or a 30x to it.
static const char *PROBE_URL = "http://connectivitycheck.gstatic.com/generate_204";

Internet checkInternet(String &detail, uint32_t timeoutMs) {
    detail = "";
    if (WiFi.status() != WL_CONNECTED) return NET_NO_WIFI;
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(timeoutMs);
    http.setConnectTimeout(timeoutMs);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (!http.begin(client, PROBE_URL)) {
        detail = "bad probe URL";
        return NET_NO_INTERNET;
    }
    const char *keys[] = {"Location"};
    http.collectHeaders(keys, 1);
    int code = http.GET();
    Internet result;
    if (code == 204) {
        result = NET_ONLINE;
    } else if (code > 0) {
        result = NET_CAPTIVE;
        detail = http.header("Location");
        if (detail.isEmpty()) detail = "HTTP " + String(code);
    } else {
        // Some portals also block DNS until you sign in, so this can be one too.
        result = NET_NO_INTERNET;
        detail = HTTPClient::errorToString(code);
    }
    http.end();
    return result;
}

const char *internetName(Internet i) {
    switch (i) {
        case NET_ONLINE: return "online";
        case NET_CAPTIVE: return "captive";
        case NET_NO_INTERNET: return "no_internet";
        default: return "no_wifi";
    }
}

String internetText(Internet i) {
    switch (i) {
        case NET_ONLINE: return "Online";
        case NET_CAPTIVE: return "This Wi-Fi needs a sign-in page. Use another network or a phone hotspot.";
        case NET_NO_INTERNET: return "Joined, but no internet (or a sign-in page is blocking it)";
        default: return "Not on Wi-Fi";
    }
}

JoinResult joinNetwork(const String &ssid, const String &password, bool save, uint32_t timeoutMs) {
    JoinResult out;
    if (WiFi.isConnected() && WiFi.SSID() == ssid) {
        out.ok = true;
        out.ip = WiFi.localIP().toString();
        if (save) bruceConfig.addWifiCredential(ssid, password);
        return out;
    }
    // No PSRAM: Wi-Fi and the Bluetooth remote rarely fit together (core/radio_mem.h).
    if (!radioHasMemForWifi() && remote::bleEnabled()) {
        Serial.println("[net] turning the Bluetooth remote off to make room for Wi-Fi");
        remote::bleSetEnabled(false);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    if (!radioHasMemForWifi()) {
        stopBLEStack();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    if (!radioHasMemForWifi()) {
        out.reason = "low_memory";
        return out;
    }

    // Keep the board's own hotspot (Web UI) up if it is on.
    WiFi.mode((WiFi.getMode() & WIFI_MODE_AP) ? WIFI_AP_STA : WIFI_STA);
    if (WiFi.isConnected()) {
        WiFi.disconnect(false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    WiFi.begin(ssid.c_str(), password.length() ? password.c_str() : nullptr);

    uint32_t start = millis();
    while (!WiFi.isConnected() && millis() - start < timeoutMs) vTaskDelay(pdMS_TO_TICKS(100));

    if (!WiFi.isConnected()) {
        wl_status_t st = WiFi.status();
        out.reason = st == WL_NO_SSID_AVAIL   ? "not_found"
                     : st == WL_CONNECT_FAILED ? "wrong_password"
                                               : "timeout";
        WiFi.disconnect(false);
        return out;
    }
    wifiConnected = true; // Bruce's status bar and apps read these
    wifiIP = WiFi.localIP().toString();
    out.ok = true;
    out.ip = wifiIP;
    if (save) bruceConfig.addWifiCredential(ssid, password);
    return out;
}

} // namespace net

static uint32_t netcheckCallback(cmd *c) {
    String detail;
    net::Internet i = net::checkInternet(detail);
    serialDevice->println(net::internetText(i));
    if (i == net::NET_ONLINE || i == net::NET_NO_WIFI) return i == net::NET_ONLINE;
    if (detail.length()) serialDevice->println("  " + detail);
    return false;
}

void createNetCommand(SimpleCLI *cli) { cli->addCommand("netcheck", netcheckCallback); }
