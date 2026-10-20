#include "backend.h"

#include "../remote/relay.h"
#include "../remote/remote.h"
#include "store.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/semphr.h>
#include <mbedtls/md.h>

namespace freq {

Words reasonWords(const String &code) {
    static const struct {
        const char *code, *title, *hint;
    } WORDS[] = {
        {"already_captured", "ALREADY IN", "Already checked in. See you next time!"},
        {"unknown_card", "UNKNOWN CARD", "Not a card from this Space. Issue a new one."},
        {"card_mismatch", "CARD MISMATCH", "This card looks copied. Issue a new one."},
        {"checkin_not_set_up", "NOT SET UP", "Turn on check-in in Space settings."},
        {"unknown_node", "NOT SET UP", "Turn on check-in in Space settings."},
        {"inactive", "CHECK-IN PAUSED", "Turn check-in back on in Space settings."},
        {"not_yet_valid", "NOT OPEN YET", "This check-in has not started yet."},
        {"expired", "CHECK-IN ENDED", "This check-in has closed."},
        {"bad_signature", "LINK OUT OF DATE", "Re-pair this scanner in Space settings."},
        {"location_required", "NEEDS A PHONE", "This check-in needs the member's phone."},
        {"too_far", "TOO FAR AWAY", "This check-in only works at the venue."},
        {"capacity_reached", "FULL UP", "This check-in has reached capacity."},
        {"device_unauthorized", "NOT PAIRED", "Pair it: Space > Check-in & scanners."},
        {"rate_limited", "SLOW DOWN", "Lots of taps. Try again in 30 seconds."},
        {"not_enabled", "SCANNERS OFF", "Device scans are off on frequencylocal.com."},
        {"invalid_input", "TAP AGAIN", "Hold the card flat and still."},
        {"unknown_member", "NO SUCH MEMBER", "Check the handle and try again."},
        {"too_many_cards", "CARD LIMIT", "That member has the most cards allowed."},
        {"internal", "SERVER HICCUP", "Frequency had a hiccup. Try again."},
    };
    for (const auto &w : WORDS)
        if (code == w.code) return {w.title, w.hint};
    return {"DIDN'T WORK", code};
}

namespace {

struct Response {
    int code = 0; // HTTP status, or <0 for a connection error
    JsonDocument body;
};

ApiStatus interpret(Response &res, String &reason);

// One JSON POST to {base}{path} with the device header.
ApiStatus post(const String &path, const JsonDocument &req, Response &res, String &reason) {
    Settings &s = settings();
    if (!s.readerReady()) {
        reason = "Reader mode is off (Settings)";
        return API_DISABLED;
    }
    if (WiFi.status() != WL_CONNECTED) {
        reason = "Wi-Fi not connected";
        return API_OFFLINE;
    }
    String base = s.baseUrl;
    while (base.endsWith("/")) base.remove(base.length() - 1);
    String url = base + path;

    relay::yieldTls(); // two TLS sessions don't fit in RAM beside the UI
    WiFiClientSecure tls;
    WiFiClient plain; // http:// is allowed for a dev server on the LAN only
    tls.setInsecure(); // TODO: pin the CA once a production device API exists
    HTTPClient http;
    http.setTimeout(6000);
    http.setConnectTimeout(5000);
    bool begun = url.startsWith("https://") ? http.begin(tls, url) : http.begin(plain, url);
    if (!begun) {
        reason = "bad base URL";
        return API_REJECTED;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Frequency-Device", s.deviceId + "." + s.deviceSecret);
    String body;
    serializeJson(req, body);
    res.code = http.POST(body);
    if (res.code > 0) deserializeJson(res.body, http.getString());
    http.end();

    if (res.code < 0) {
        reason = HTTPClient::errorToString(res.code);
        return API_OFFLINE;
    }
    return interpret(res, reason);
}

// What the server's answer means for the device. Shared by Wi-Fi and the cable.
ApiStatus interpret(Response &res, String &reason) {
    if (res.code == 404) {
        reason = "not_enabled";
        return API_NOT_SET_UP;
    }
    if (res.code >= 500) {
        reason = "server error " + String(res.code);
        return API_OFFLINE;
    }
    String r = res.body["reason"] | "";
    if (res.code >= 400) {
        reason = r.length() ? r : "HTTP " + String(res.code);
        return API_REJECTED;
    }
    if (!(res.body["ok"] | false)) {
        reason = r.length() ? r : "rejected";
        return API_REJECTED;
    }
    return API_OK;
}

void readScan(Response &res, ScanReply &out) {
    out.name = res.body["member"]["display_name"] | "Member";
    out.zaps = res.body["zapsAwarded"] | 0;
    JsonVariant loyalty = res.body["loyalty"];
    if (!loyalty.isNull()) {
        out.hasLoyalty = true;
        out.stamps = loyalty["stamps"] | 0;
        out.required = loyalty["required"] | 0;
        out.earned = loyalty["earned"] | false;
        out.rewardCode = loyalty["reward_code"] | "";
    }
}

// ---- scans through the cable (PROTOCOL.md "Scans through the cable") -------------

struct Relay {
    SemaphoreHandle_t done = xSemaphoreCreateBinary();
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    uint32_t nonce = 0; // the scan waiting for its answer, 0 = none
    int code = 0;
    String body;        // the server's JSON reply, as the computer passed it on
};
Relay &relay() {
    static Relay r;
    return r;
}

String hex(const uint8_t *b, size_t n) {
    static const char *H = "0123456789abcdef";
    String s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; i++) {
        s += H[b[i] >> 4];
        s += H[b[i] & 15];
    }
    return s;
}

} // namespace

SignedScan signScan(const String &token, const String &uid, uint32_t scannedAt) {
    SignedScan out;
    Settings &s = settings();
    out.deviceId = s.deviceId;
    out.nonce = esp_random() | 1;
    JsonDocument req;
    req["card_token"] = token;
    req["uid"] = uid;
    req["scanned_at"] = scannedAt;
    char nonce[9];
    snprintf(nonce, sizeof(nonce), "%08lx", (unsigned long)out.nonce);
    req["nonce"] = nonce;
    serializeJson(req, out.body);

    // Key = sha256(secret): the server keeps exactly that hash, so it can check
    // the signature without ever holding the secret itself.
    const mbedtls_md_info_t *sha = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    uint8_t key[32], mac[32];
    mbedtls_md(sha, (const uint8_t *)s.deviceSecret.c_str(), s.deviceSecret.length(), key);
    String msg = "scan:v1:" + s.deviceId + ":" + out.body;
    mbedtls_md_hmac(sha, key, sizeof(key), (const uint8_t *)msg.c_str(), msg.length(), mac);
    memset(key, 0, sizeof(key));
    out.signature = "v1=" + hex(mac, sizeof(mac));
    return out;
}

bool cableAvailable() {
    return WiFi.status() != WL_CONNECTED && remote::usbHostActive() && remote::trustedEpoch() != 0 &&
           settings().readerReady();
}

void putRelay(JsonObject o, const SignedScan &s) {
    char nonce[9];
    snprintf(nonce, sizeof(nonce), "%08lx", (unsigned long)s.nonce);
    o["nonce"] = nonce;
    o["device_id"] = s.deviceId;
    o["body"] = s.body;
    o["signature"] = s.signature;
}

bool relayAnswer(uint32_t nonce, int httpCode, const String &body) {
    Relay &r = relay();
    bool match = false;
    portENTER_CRITICAL(&r.mux);
    if (nonce && r.nonce == nonce) {
        r.nonce = 0;
        match = true;
    }
    portEXIT_CRITICAL(&r.mux);
    if (!match) return false;
    r.code = httpCode;
    r.body = body;
    xSemaphoreGive(r.done);
    return true;
}

namespace {

// Hands a signed scan to the computer on the cable and waits for the server's
// answer (frequency.relay_reply). Must not run on the CLI task: that task is the
// one that would deliver the answer.
ApiStatus relayScan(const String &token, const String &uid, Response &res, String &reason) {
    SignedScan sc = signScan(token, uid, remote::trustedEpoch());
    Relay &r = relay();
    xSemaphoreTake(r.done, 0); // drop a stale answer
    portENTER_CRITICAL(&r.mux);
    r.nonce = sc.nonce;
    portEXIT_CRITICAL(&r.mux);

    JsonDocument evt;
    putRelay(evt.to<JsonObject>(), sc);
    remote::emit("frequency.scan_relay", evt.as<JsonObjectConst>());

    bool answered = xSemaphoreTake(r.done, pdMS_TO_TICKS(RELAY_TIMEOUT_MS)) == pdTRUE;
    portENTER_CRITICAL(&r.mux);
    r.nonce = 0;
    portEXIT_CRITICAL(&r.mux);
    if (!answered) {
        reason = "the computer didn't answer";
        return API_OFFLINE;
    }
    res.code = r.code;
    if (r.code > 0) deserializeJson(res.body, r.body);
    if (res.code <= 0) {
        reason = "the computer is offline too";
        return API_OFFLINE;
    }
    return interpret(res, reason);
}

} // namespace

ScanReply postScan(const String &token, const String &uid, bool queued, uint32_t scannedAt) {
    ScanReply out;
    Response res;
    if (!queued && cableAvailable()) {
        out.status = relayScan(token, uid, res, out.reason);
        out.viaCable = true;
    } else {
        JsonDocument req;
        req["card_token"] = token;
        req["uid"] = uid;
        if (queued) { // an offline scan being replayed
            req["queued"] = true;
            if (scannedAt) req["scanned_at"] = scannedAt;
        }
        out.status = post("/api/v1/devices/scan", req, res, out.reason);
    }
    if (out.status == API_OK) readScan(res, out);
    return out;
}

IssueReply issueCard(const String &memberHandle) {
    IssueReply out;
    JsonDocument req;
    req["member_handle"] = memberHandle;
    Response res;
    out.status = post("/api/v1/devices/cards", req, res, out.reason);
    if (out.status != API_OK) return out;
    out.token = res.body["token"] | "";
    out.url = res.body["url"] | "";
    if (out.url.isEmpty()) {
        out.status = API_REJECTED;
        out.reason = "reply had no url";
    }
    return out;
}

int retryQueue(String &err) {
    std::vector<QueuedScan> q = queuedScans(), done;
    int answered = 0;
    for (const QueuedScan &s : q) {
        ScanReply r = postScan(s.token, s.uid, true, s.ts);
        // A 401 (scanner not paired / wrong secret) or 429 (slow down) says nothing
        // about the scan itself: keep it and the rest for the next try.
        bool retryLater = r.reason == "device_unauthorized" || r.reason == "rate_limited" ||
                          r.reason == "HTTP 401" || r.reason == "HTTP 429";
        if (r.status == API_REJECTED && retryLater) {
            err = r.reason;
            break;
        }
        if (r.status == API_OK || r.status == API_REJECTED) {
            done.push_back(s); // the server decided; don't send it twice
            answered++;
        } else {
            err = r.reason;
            break;
        }
    }
    removeQueued(done);
    return answered;
}

} // namespace freq
