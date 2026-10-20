#include "backend.h"

#include "store.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

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

} // namespace

ScanReply postScan(const String &token, const String &uid, bool queued, uint32_t scannedAt) {
    ScanReply out;
    JsonDocument req;
    req["card_token"] = token;
    req["uid"] = uid;
    if (queued) { // proposed extension: an offline scan being replayed
        req["queued"] = true;
        if (scannedAt) req["scanned_at"] = scannedAt;
    }
    Response res;
    out.status = post("/api/v1/devices/scan", req, res, out.reason);
    if (out.status != API_OK) return out;
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
