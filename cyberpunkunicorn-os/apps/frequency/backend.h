// Frequency device API client. The endpoints are a PROPOSED contract
// (apps/frequency/BACKEND.md): frequency-web does not implement them yet, so
// a 404 means "backend not set up for devices", not "card unknown".
#pragma once

#include <Arduino.h>

namespace freq {

enum ApiStatus {
    API_OK,
    API_REJECTED,  // server answered ok:false or 4xx: show `reason`
    API_NOT_SET_UP, // 404: the device endpoints don't exist yet
    API_OFFLINE,   // no Wi-Fi, connection failed or 5xx: worth queueing
    API_DISABLED,  // reader mode off or device id/secret missing
};
extern const char *NOT_SET_UP_TEXT;

struct ScanReply {
    ApiStatus status = API_DISABLED;
    String reason;
    String name;
    int zaps = 0;
    bool hasLoyalty = false;
    int stamps = 0, required = 0;
    bool earned = false;
    String rewardCode;
};
// POST {base}/api/v1/devices/scan. `queued` marks a replay from the offline
// queue; `scannedAt` (epoch s) is sent with it when the clock was set.
ScanReply postScan(const String &token, const String &uid, bool queued = false, uint32_t scannedAt = 0);

struct IssueReply {
    ApiStatus status = API_DISABLED;
    String reason;
    String token, url;
};
// POST {base}/api/v1/devices/cards
IssueReply issueCard(const String &memberHandle);

// Replays the offline queue. Returns scans the server answered (accepted or
// rejected); stops at the first one that is still offline.
int retryQueue(String &err);

} // namespace freq
