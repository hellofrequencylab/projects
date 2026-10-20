// Frequency device API client: POST /api/v1/devices/scan and /cards on
// frequency-web (LIVE-1125), authenticated with X-Frequency-Device. While the
// site's scanner switch is off both answer 404, which means "device scans are
// switched off", not "card unknown". Refusals come back as reason codes;
// reasonWords() turns them into screen text.
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace freq {

enum ApiStatus {
    API_OK,
    API_REJECTED,  // server answered ok:false or 4xx: show `reason`
    API_NOT_SET_UP, // 404: device scans are switched off on the site
    API_OFFLINE,   // no Wi-Fi, connection failed or 5xx: worth queueing
    API_DISABLED,  // reader mode off or device id/secret missing
};

// Plain words for a refusal code, for the screen: a short title (ASCII caps,
// fits size 2) and one instruction. Unknown codes come back as themselves.
struct Words {
    const char *title;
    String hint;
};
Words reasonWords(const String &code);

struct ScanReply {
    ApiStatus status = API_DISABLED;
    String reason;
    String name;
    int zaps = 0;
    bool hasLoyalty = false;
    int stamps = 0, required = 0;
    bool earned = false;
    String rewardCode;
    bool viaCable = false; // went through the computer on the USB cable, not Wi-Fi
};
// POST {base}/api/v1/devices/scan. `queued` marks a replay from the offline
// queue; `scannedAt` (epoch s) is sent with it when the clock was set.
// With no Wi-Fi but a computer on the USB cable (cableAvailable()), a fresh scan
// goes through that computer instead. Call this from the UI task, never from a
// remote command handler: the CLI task delivers the computer's answer.
ScanReply postScan(const String &token, const String &uid, bool queued = false, uint32_t scannedAt = 0);

struct IssueReply {
    ApiStatus status = API_DISABLED;
    String reason;
    String token, url;
};
// POST {base}/api/v1/devices/cards
IssueReply issueCard(const String &memberHandle);

// ---- scans through the cable (Phase 5; PROTOCOL.md "Scans through the cable") ----
// The board signs the scan with its secret; the computer forwards the body and
// signature to the server unchanged, so the secret never leaves the board.
constexpr uint32_t RELAY_TIMEOUT_MS = 12000;
struct SignedScan {
    String deviceId;
    String body;      // the exact JSON to POST; the signature covers these bytes
    String signature; // "v1=<hex HMAC-SHA256>"
    uint32_t nonce = 0;
};
SignedScan signScan(const String &token, const String &uid, uint32_t scannedAt);
void putRelay(JsonObject o, const SignedScan &s); // {nonce, device_id, body, signature}
// No Wi-Fi, a computer talking over USB, its clock set (system.time), reader ready.
bool cableAvailable();
// frequency.relay_reply: the server's answer for the scan waiting on `nonce`.
// False if no scan is waiting on it (late, or already answered).
bool relayAnswer(uint32_t nonce, int httpCode, const String &body);

// Replays the offline queue. Returns scans the server answered (accepted or
// rejected); stops at the first one that is still offline.
int retryQueue(String &err);

} // namespace freq
