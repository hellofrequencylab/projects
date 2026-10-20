// Console Wi-Fi setup over the remote protocol (apps/remote/PROTOCOL.md "Wi-Fi"):
// the Frequency console on a computer lists networks and sends the password over
// USB, so nobody types it on the resistive keyboard. These run on the CLI task:
// no drawing, and every wait is bounded.
#include "../remote/remote.h"
#include "core/radio_mem.h"
#include "core/settings.h"
#include "net.h"
#include <WiFi.h>
#include <globals.h>

namespace {

const char *authName(wifi_auth_mode_t a) {
    switch (a) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "wep";
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK: return "wpa2";
        case WIFI_AUTH_WPA3_PSK:
        case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa3";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "enterprise"; // the board can't join these
        default: return "other";
    }
}

// ---- wifi.status {check?} ----------------------------------------------------------

void cmdWifiStatus(remote::Request &r) {
    bool sta = WiFi.status() == WL_CONNECTED;
    r.data["connected"] = sta;
    r.data["ssid"] = sta ? WiFi.SSID() : String("");
    r.data["ip"] = sta ? WiFi.localIP().toString() : String("");
    if (sta) {
        r.data["rssi"] = WiFi.RSSI();
        r.data["channel"] = WiFi.channel();
    }
    r.data["mac"] = WiFi.macAddress();
    if (WiFi.getMode() & WIFI_MODE_AP) r.data["ap_ip"] = WiFi.softAPIP().toString();
    JsonArray saved = r.data["saved"].to<JsonArray>(); // names only, never passwords
    for (const auto &kv : bruceConfig.wifi) saved.add(kv.first);
    if (r.args["check"] | false) {
        String detail;
        net::Internet i = net::checkInternet(detail);
        r.data["internet"] = net::internetName(i);
        if (detail.length() && i != net::NET_ONLINE) r.data["internet_detail"] = detail;
    }
}

// ---- wifi.scan ---------------------------------------------------------------------

void cmdWifiScan(remote::Request &r) {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        if (!radioHasMemForWifi()) return r.fail("low_memory", "turn Bluetooth off first");
        WiFi.mode(WIFI_STA);
    }
    int n = WiFi.scanNetworks(); // blocking, ~2-4 s
    if (n < 0) return r.fail("scan_failed", "scan error " + String(n));
    JsonArray nets = r.data["networks"].to<JsonArray>();
    for (int i = 0; i < n && i < 30; i++) {
        String ssid = WiFi.SSID(i);
        JsonObject o = nets.add<JsonObject>();
        o["ssid"] = ssid; // "" = hidden network
        o["rssi"] = WiFi.RSSI(i);
        o["channel"] = WiFi.channel(i);
        o["auth"] = authName(WiFi.encryptionType(i));
        o["saved"] = ssid.length() && bruceConfig.hasWifiCredential(ssid);
    }
    WiFi.scanDelete();
}

// ---- wifi.join {ssid, password?, save?, timeout_ms?} ---------------------------------

void cmdWifiJoin(remote::Request &r) {
    String ssid = r.args["ssid"] | "";
    String password = r.args["password"] | "";
    bool save = r.args["save"] | true;
    uint32_t timeout = r.args["timeout_ms"] | 20000;
    if (ssid.isEmpty() || ssid.length() > 32) return r.fail("bad_args", "ssid is 1-32 characters");
    if (password.length() > 63) return r.fail("bad_args", "password is at most 63 characters");
    if (password.length() > 0 && password.length() < 8) return r.fail("bad_args", "a WPA2 password is 8-63 characters");
    if (timeout < 5000) timeout = 5000;
    if (timeout > 30000) timeout = 30000;

    remote::emit("wifi.joining", JsonObjectConst());
    net::JoinResult j = net::joinNetwork(ssid, password, save, timeout);
    if (!j.ok) {
        static const struct {
            const char *code, *text;
        } WHY[] = {
            {"not_found", "network not found (is it 2.4 GHz and in range?)"},
            {"wrong_password", "the network refused the password"},
            {"timeout", "no answer: check the password, or the network is slow to give an address"},
            {"low_memory", "not enough memory for Wi-Fi: free Bluetooth or SD first"},
        };
        for (const auto &w : WHY)
            if (j.reason == w.code) return r.fail(w.code, w.text);
        return r.fail("join_failed", j.reason);
    }
    r.data["ssid"] = ssid;
    r.data["ip"] = j.ip;
    r.data["saved"] = save;
    String detail;
    net::Internet i = net::checkInternet(detail);
    r.data["internet"] = net::internetName(i);
    if (i == net::NET_CAPTIVE && detail.length()) r.data["portal"] = detail;
}

// ---- wifi.forget {ssid} --------------------------------------------------------------

void cmdWifiForget(remote::Request &r) {
    String ssid = r.args["ssid"] | "";
    if (ssid.isEmpty()) return r.fail("bad_args", "ssid is required");
    if (!bruceConfig.hasWifiCredential(ssid)) return r.fail("not_saved", "that network isn't saved");
    bruceConfig.removeWifiCredential(ssid);
    r.data["ssid"] = ssid;
}

} // namespace

REMOTE_COMMAND("wifi.status", cmdWifiStatus);
REMOTE_COMMAND("wifi.scan", cmdWifiScan);
REMOTE_COMMAND("wifi.join", cmdWifiJoin);
REMOTE_COMMAND("wifi.forget", cmdWifiForget);
