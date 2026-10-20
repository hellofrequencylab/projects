#include "store.h"

#include <LittleFS.h>
#include <Preferences.h>
#include <SD.h>
#include <freertos/semphr.h>
#include <globals.h>

namespace freq {

const char *DEFAULT_BASE_URL = "https://frequencylocal.com";
const char *TAG_LIST_PATH = "/frequency/tags.txt";

namespace {

const char *FREQ_DIR = "/frequency";
const char *WRITTEN = "/frequency/written.csv";
const char *WRITTEN_TMP = "/frequency/written.tmp";
const char *QUEUE = "/frequency/scanq.csv";
const char *QUEUE_TMP = "/frequency/scanq.tmp";
constexpr size_t WRITTEN_CAP = 16 * 1024;
constexpr size_t FS_RESERVE = 8 * 1024; // LittleFS also holds Bruce's config: never fill it

SemaphoreHandle_t lock() {
    static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    return m;
}
struct Guard {
    Guard() { xSemaphoreTakeRecursive(lock(), portMAX_DELAY); }
    ~Guard() { xSemaphoreGiveRecursive(lock()); }
};

Settings cfg;
bool cfgLoaded = false;

bool fsRoomFor(size_t bytes) { return LittleFS.totalBytes() - LittleFS.usedBytes() >= bytes + FS_RESERVE; }

size_t fileSize(const char *path) {
    File f = LittleFS.open(path, FILE_READ);
    if (!f) return 0;
    size_t s = f.size();
    f.close();
    return s;
}

// Keep the newer half of a log once it passes `cap`.
void trimLog(const char *path, const char *tmp, size_t cap) {
    if (fileSize(path) <= cap) return;
    File in = LittleFS.open(path, FILE_READ);
    if (!in) return;
    in.seek(in.size() - cap / 2);
    in.readStringUntil('\n'); // drop the partial line
    File out = LittleFS.open(tmp, FILE_WRITE);
    while (out && in.available()) out.write(in.read());
    in.close();
    if (out) out.close();
    LittleFS.remove(path);
    LittleFS.rename(tmp, path);
}

String noCommas(String s) {
    s.replace(",", " ");
    s.replace("\n", " ");
    s.replace("\r", "");
    return s;
}

bool isHex(char c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool isB64url(char c) { return isalnum(c) || c == '-' || c == '_'; }

bool isUuid(const String &s) {
    if (s.length() != 36) return false;
    for (int i = 0; i < 36; i++) {
        bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? s[i] != '-' : !isHex(s[i])) return false;
    }
    return true;
}

// "v1.<iat base36>.<tag base64url, 16 bytes = 22 chars>"
bool isNodeSig(const String &s) {
    if (!s.startsWith("v1.")) return false;
    int dot = s.indexOf('.', 3);
    if (dot <= 3) return false;
    for (int i = 3; i < dot; i++)
        if (!isalnum(s[i])) return false;
    String tag = s.substring(dot + 1);
    if (tag.length() != 22) return false;
    for (char c : tag)
        if (!isB64url(c)) return false;
    return true;
}

String queryParam(const String &query, const String &key) {
    int start = 0;
    while (start < (int)query.length()) {
        int amp = query.indexOf('&', start);
        if (amp < 0) amp = query.length();
        String kv = query.substring(start, amp);
        if (kv.startsWith(key + "=")) return kv.substring(key.length() + 1);
        start = amp + 1;
    }
    return "";
}

} // namespace

// ---- settings -----------------------------------------------------------------

void Settings::load() {
    Preferences p;
    p.begin("frequency", true);
    readerMode = p.getBool("reader", false);
    baseUrl = p.getString("base", DEFAULT_BASE_URL);
    deviceId = p.getString("devid", "");
    deviceSecret = p.getString("devsecret", "");
    p.end();
}

void Settings::save() const {
    Preferences p;
    p.begin("frequency", false);
    p.putBool("reader", readerMode);
    p.putString("base", baseUrl);
    p.putString("devid", deviceId);
    p.putString("devsecret", deviceSecret);
    p.end();
}

Settings &settings() {
    Guard g;
    if (!cfgLoaded) {
        cfg.load();
        cfgLoaded = true;
    }
    return cfg;
}

uint32_t nowEpoch() { return clock_set ? (uint32_t)rtc.getEpoch() : 0; }

bool storeBegin() {
    Guard g;
    // Bruce mounts LittleFS at boot; never format here, its config lives there too.
    if (!LittleFS.begin(false)) return false;
    if (!LittleFS.exists(FREQ_DIR)) LittleFS.mkdir(FREQ_DIR);
    return true;
}

// ---- URLs ---------------------------------------------------------------------

const char *kindName(UrlKind k) {
    switch (k) {
        case URL_EMPTY: return "empty";
        case URL_VENUE: return "venue check-in";
        case URL_EVENT: return "event door";
        case URL_MEMBER: return "member card";
        default: return "other";
    }
}

UrlCheck checkUrl(const String &raw) {
    UrlCheck u;
    String url = raw;
    url.trim();
    u.fixed = url;
    if (url.isEmpty()) return u;
    u.kind = URL_OTHER;
    int scheme = url.indexOf("://");
    if (scheme < 0 || !(url.startsWith("https://") || url.startsWith("http://"))) {
        u.problem = "not an http(s) URL";
        return u;
    }
    int pathAt = url.indexOf('/', scheme + 3);
    u.host = url.substring(scheme + 3, pathAt < 0 ? url.length() : pathAt);
    String rest = pathAt < 0 ? "/" : url.substring(pathAt);
    int q = rest.indexOf('?');
    u.path = q < 0 ? rest : rest.substring(0, q);
    String query = q < 0 ? "" : rest.substring(q + 1);
    if (u.host.isEmpty()) {
        u.problem = "no host";
        return u;
    }

    if (u.path.startsWith("/n/")) {
        u.kind = URL_VENUE;
        String node = u.path.substring(3);
        if (!isUuid(node)) u.problem = "node id is not a UUID";
        else if (!isNodeSig(queryParam(query, "s"))) u.problem = "missing or bad ?s=v1... signature";
        else if (queryParam(query, "m").length() && queryParam(query, "m") != "nfc") u.problem = "?m= on a /n/ link must be nfc";
        else {
            u.ok = true;
            // Placards carry ?m=nfc too, so the site's Scan stats count them as taps.
            if (queryParam(query, "m") != "nfc") u.fixed = url + "&m=nfc";
        }
    } else if (u.path.startsWith("/q/")) {
        u.kind = URL_EVENT;
        String slug = u.path.substring(3);
        bool good = slug.length() == 7;
        for (char c : slug) good = good && isB64url(c);
        if (!good) u.problem = "event slug should be 7 characters";
        else {
            u.ok = true;
            // NFC tags carry ?m=nfc so the site knows the tap came from a tag.
            if (queryParam(query, "m") != "nfc") u.fixed = url + (q < 0 ? "?m=nfc" : "&m=nfc");
        }
    } else if (u.path.startsWith("/c/")) {
        u.kind = URL_MEMBER;
        u.token = u.path.substring(3);
        bool good = u.token.length() >= 16;
        for (char c : u.token) good = good && isB64url(c);
        if (!good) u.problem = "bad member card token";
        else u.ok = true;
    } else {
        u.problem = "not a Frequency /n/, /q/ or /c/ link";
    }
    return u;
}

// ---- tags.txt -------------------------------------------------------------------

std::vector<TagSource> loadTagList(String &from) {
    Guard g;
    std::vector<TagSource> list;
    File f;
    from = "";
    if (sdcardMounted && SD.exists(TAG_LIST_PATH)) {
        f = SD.open(TAG_LIST_PATH, FILE_READ);
        from = "SD";
    }
    if (!f && LittleFS.exists(TAG_LIST_PATH)) {
        f = LittleFS.open(TAG_LIST_PATH, FILE_READ);
        from = "LittleFS";
    }
    if (!f) return list;
    while (f.available() && list.size() < 64) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty() || line.startsWith("#")) continue;
        int bar = line.indexOf('|');
        TagSource t;
        if (bar < 0) {
            t.url = line;
            t.label = line;
        } else {
            t.label = line.substring(0, bar);
            t.url = line.substring(bar + 1);
        }
        t.label.trim();
        t.url.trim();
        if (t.url.length()) list.push_back(t);
    }
    f.close();
    return list;
}

// ---- written log --------------------------------------------------------------------

bool logWrite(const String &label, const UrlCheck &u, const String &uid, bool locked) {
    Guard g;
    if (!storeBegin() || !fsRoomFor(256)) return false;
    trimLog(WRITTEN, WRITTEN_TMP, WRITTEN_CAP);
    bool fresh = !LittleFS.exists(WRITTEN);
    File f = LittleFS.open(WRITTEN, FILE_APPEND);
    if (!f) return false;
    if (fresh) f.println("ts,uid,kind,label,url,locked");
    // Host + path only: the ?s= signature is what makes a node link valid, so it
    // stays on the tag and off the log.
    f.printf(
        "%lu,%s,%s,%s,%s%s,%d\n",
        (unsigned long)nowEpoch(),
        uid.c_str(),
        kindName(u.kind),
        noCommas(label).c_str(),
        noCommas(u.host).c_str(),
        noCommas(u.path).c_str(),
        locked ? 1 : 0
    );
    f.close();
    return true;
}

std::vector<String> recentWrites(int n) {
    Guard g;
    std::vector<String> ring;
    File f = LittleFS.open(WRITTEN, FILE_READ);
    if (!f) return ring;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty() || line.startsWith("ts,")) continue;
        ring.push_back(line);
        if ((int)ring.size() > n) ring.erase(ring.begin());
    }
    f.close();
    return ring;
}

// ---- offline scan queue -------------------------------------------------------------

std::vector<QueuedScan> queuedScans() {
    Guard g;
    std::vector<QueuedScan> v;
    File f = LittleFS.open(QUEUE, FILE_READ);
    if (!f) return v;
    while (f.available() && (int)v.size() < QUEUE_MAX) {
        String line = f.readStringUntil('\n');
        line.trim();
        int a = line.indexOf(','), b = line.indexOf(',', a + 1);
        if (a < 0 || b < 0) continue;
        v.push_back({(uint32_t)strtoul(line.c_str(), nullptr, 10), line.substring(a + 1, b), line.substring(b + 1)});
    }
    f.close();
    return v;
}

bool queueScan(const QueuedScan &s) {
    Guard g;
    if (!storeBegin() || !fsRoomFor(256)) return false;
    if ((int)queuedScans().size() >= QUEUE_MAX) return false;
    File f = LittleFS.open(QUEUE, FILE_APPEND);
    if (!f) return false;
    f.printf("%lu,%s,%s\n", (unsigned long)s.ts, s.uid.c_str(), noCommas(s.token).c_str());
    f.close();
    return true;
}

void removeQueued(const std::vector<QueuedScan> &done) {
    Guard g;
    // Re-read under the lock: a scan queued while the retry was posting stays.
    std::vector<QueuedScan> keep;
    for (const QueuedScan &q : queuedScans()) {
        bool sent = false;
        for (const QueuedScan &d : done) sent = sent || (d.ts == q.ts && d.token == q.token && d.uid == q.uid);
        if (!sent) keep.push_back(q);
    }
    if (keep.empty()) {
        LittleFS.remove(QUEUE);
        return;
    }
    File f = LittleFS.open(QUEUE_TMP, FILE_WRITE);
    if (!f) return;
    for (const QueuedScan &q : keep) f.printf("%lu,%s,%s\n", (unsigned long)q.ts, q.uid.c_str(), q.token.c_str());
    f.close();
    LittleFS.remove(QUEUE);
    LittleFS.rename(QUEUE_TMP, QUEUE);
}

} // namespace freq
