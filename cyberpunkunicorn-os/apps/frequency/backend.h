// Frequency device API client: POST /api/v1/devices/scan and /cards on
// frequency-web (LIVE-1125), authenticated with X-Frequency-Device. While the
// site's scanner switch is off both answer 404, which means "device scans are
// switched off", not "card unknown". Refusals come back as reason codes;
// reasonWords() turns them into screen text.
#pragma once

#include <Arduino.h>

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
