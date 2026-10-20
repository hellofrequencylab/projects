// Frequency local state: settings (NVS namespace "frequency"), URL checks,
// the tags.txt pick list, the log of tags written and the offline scan queue.
//
// The loyalty ledger is NOT here: frequency-web (Supabase) is the source of
// truth. This device only writes venue tags and, in reader mode, forwards
// member-card scans. The CLI runs on its own FreeRTOS task, so every function
// here that touches files takes the store lock itself.
#pragma once

#include <Arduino.h>
#include <vector>

namespace freq {

struct Settings {
    bool readerMode = false; // member-card scanning (proposed backend, off by default)
    String baseUrl;          // frequency-web origin, e.g. https://frequencylocal.com
    String deviceId;
    String deviceSecret;

    void load();
    void save() const;
    bool readerReady() const { return readerMode && baseUrl.length() && deviceId.length() && deviceSecret.length(); }
};
extern const char *DEFAULT_BASE_URL;
Settings &settings(); // loaded on first use

// ---- URLs -------------------------------------------------------------------
enum UrlKind {
    URL_EMPTY,
    URL_VENUE,  // /n/<node uuid>?s=v1.<iat>.<tag>[&m=nfc]  Space check-in node
    URL_EVENT,  // /q/<7-char slug>[?m=nfc]          event door code
    URL_MEMBER, // /c/<token>                         member card (proposed)
    URL_OTHER,
};
struct UrlCheck {
    UrlKind kind = URL_EMPTY;
    bool ok = false;     // well formed for its kind
    String problem;      // why not, for the warning screen
    String fixed;        // url to actually write (/n/ and /q/ get m=nfc)
    String host;         // "frequencylocal.com"
    String path;         // "/n/<uuid>" (no query: the signature stays off logs)
    String token;        // member token for URL_MEMBER
};
UrlCheck checkUrl(const String &url);
const char *kindName(UrlKind k);

// ---- tags.txt: "label|url" per line, SD first, then LittleFS ------------------
struct TagSource {
    String label;
    String url;
};
std::vector<TagSource> loadTagList(String &from); // `from` = "SD" / "LittleFS" / ""
extern const char *TAG_LIST_PATH;

// ---- log of tags written (/frequency/written.csv) ----------------------------
bool logWrite(const String &label, const UrlCheck &u, const String &uid, bool locked);
std::vector<String> recentWrites(int n); // newest last

// ---- offline scan queue (/frequency/scanq.csv) ---------------------------------
constexpr int QUEUE_MAX = 100;
struct QueuedScan {
    uint32_t ts;
    String uid;
    String token;
};
bool queueScan(const QueuedScan &s); // false = queue full
std::vector<QueuedScan> queuedScans();
void removeQueued(const std::vector<QueuedScan> &done); // drop the ones the server took

bool storeBegin();   // ensures /frequency exists on LittleFS
uint32_t nowEpoch(); // 0 when the clock has not been set

} // namespace freq
