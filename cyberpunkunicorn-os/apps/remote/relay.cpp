#include "relay.h"

#include "../frequency/store.h"
#include "remote.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <freertos/semphr.h>
#include <mbedtls/base64.h>
#include <vector>

namespace relay {

namespace {

constexpr uint32_t HEARTBEAT_MS = 25000;
constexpr uint32_t MIN_HEAP = 60000;     // TLS takes ~40 KB while open; leave the UI room
constexpr uint32_t REMINT_EARLY_S = 120; // re-mint this long before the token expires
constexpr uint32_t YIELD_MS = 20000;     // how long yieldTls() keeps the socket closed
constexpr size_t MAX_MESSAGE = 4096;     // a Realtime message bigger than this is dropped

volatile State st = OFF;
volatile bool wantOn = false;
volatile uint32_t yieldUntil = 0;
TaskHandle_t task = nullptr;

SemaphoreHandle_t lock() { // guards the socket writes, err and allowed
    static SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    return m;
}
struct Guard {
    Guard() { xSemaphoreTakeRecursive(lock(), portMAX_DELAY); }
    ~Guard() { xSemaphoreGiveRecursive(lock()); }
};

String err;
std::vector<String> allowed; // from the token reply: the owner's allow-list
std::vector<String> pendingEvents; // @evt JSON waiting for the relay task to send
volatile bool yieldReq = false;
// Only the relay task touches the socket: TLS can't read and write from two tasks.
WiFiClientSecure *sock = nullptr;
String topic; // "realtime:device:<id>"
uint32_t refNo = 0;

void setErr(const String &e) {
    Guard g;
    err = e;
}

// ---- WebSocket (RFC 6455), client side only ---------------------------------------

bool wsSend(uint8_t opcode, const String &payload) {
    if (!sock || !sock->connected()) return false;
    size_t n = payload.length();
    uint8_t head[14];
    size_t h = 0;
    head[h++] = 0x80 | opcode; // FIN
    if (n < 126) head[h++] = 0x80 | n;
    else if (n < 65536) {
        head[h++] = 0x80 | 126;
        head[h++] = n >> 8;
        head[h++] = n & 0xff;
    } else return false;
    uint32_t mask = esp_random();
    uint8_t m[4] = {uint8_t(mask), uint8_t(mask >> 8), uint8_t(mask >> 16), uint8_t(mask >> 24)};
    memcpy(head + h, m, 4);
    h += 4;
    if (sock->write(head, h) != h) return false;
    // Mask in small chunks: no second copy of a big payload on the heap.
    uint8_t buf[128];
    for (size_t i = 0; i < n; i += sizeof(buf)) {
        size_t k = min(sizeof(buf), n - i);
        for (size_t j = 0; j < k; j++) buf[j] = payload[i + j] ^ m[(i + j) & 3];
        if (sock->write(buf, k) != k) return false;
    }
    return true;
}

bool readExact(uint8_t *dst, size_t n, uint32_t timeoutMs) {
    uint32_t start = millis();
    size_t got = 0;
    while (got < n) {
        if (!sock->connected()) return false;
        int a = sock->available();
        if (a > 0) {
            int r = sock->read(dst + got, min((size_t)a, n - got));
            if (r > 0) got += r;
        } else {
            if (millis() - start > timeoutMs) return false;
            delay(5);
        }
    }
    return true;
}

// Reads one whole message (joining fragments), answering pings on the way.
// 1 = text in `out`, 0 = nothing waiting, -1 = closed or broken.
int wsRead(String &out) {
    if (!sock->connected()) return -1;
    if (sock->available() < 2) return 0;
    out = "";
    for (;;) {
        uint8_t h[2];
        if (!readExact(h, 2, 5000)) return -1;
        bool fin = h[0] & 0x80;
        uint8_t op = h[0] & 0x0f;
        uint64_t len = h[1] & 0x7f;
        if (len == 126) {
            uint8_t e[2];
            if (!readExact(e, 2, 5000)) return -1;
            len = (e[0] << 8) | e[1];
        } else if (len == 127) {
            uint8_t e[8];
            if (!readExact(e, 8, 5000)) return -1;
            len = 0;
            for (int i = 0; i < 8; i++) len = (len << 8) | e[i];
        }
        if (h[1] & 0x80) return -1; // servers never mask
        bool keep = out.length() + len <= MAX_MESSAGE;
        String chunk;
        uint8_t buf[128];
        for (uint64_t i = 0; i < len; i += sizeof(buf)) {
            size_t k = min((uint64_t)sizeof(buf), len - i);
            if (!readExact(buf, k, 5000)) return -1;
            if (keep) chunk.concat((const char *)buf, k);
        }
        if (op == 0x8) return -1;                          // close
        if (op == 0x9) { wsSend(0xA, chunk); continue; }   // ping -> pong
        if (op == 0xA) { if (!out.length()) return 0; continue; }
        if (op == 0x1 || op == 0x0) {
            if (!keep) out = "\x01"; // too big: remembered, then dropped below
            else if (out != "\x01") out += chunk;
            if (!fin) continue;
            if (out == "\x01") { out = ""; return 0; }
            return 1;
        }
        // binary or unknown: ignore
        if (fin) return 0;
    }
}

bool wsOpen(const String &wssUrl, const String &apikey) {
    // wss://host/path
    if (!wssUrl.startsWith("wss://")) return false;
    String rest = wssUrl.substring(6);
    int slash = rest.indexOf('/');
    String host = slash < 0 ? rest : rest.substring(0, slash);
    String path = slash < 0 ? String("/") : rest.substring(slash);
    path += (path.indexOf('?') < 0 ? "?" : "&") + String("apikey=") + apikey + "&vsn=1.0.0";

    WiFiClientSecure *c = new (std::nothrow) WiFiClientSecure();
    if (!c) return false;
    c->setInsecure(); // TODO with the scanner: pin the CA bundle (plan Phase 3)
    c->setTimeout(10);
    if (!c->connect(host.c_str(), 443)) {
        delete c;
        setErr("couldn't reach " + host);
        return false;
    }
    uint8_t raw[16];
    for (int i = 0; i < 16; i += 4) {
        uint32_t r = esp_random();
        memcpy(raw + i, &r, 4);
    }
    unsigned char key[32];
    size_t klen = 0;
    mbedtls_base64_encode(key, sizeof(key), &klen, raw, sizeof(raw));
    key[klen] = 0;
    String req = "GET " + path + " HTTP/1.1\r\nHost: " + host +
                 "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + String((char *)key) +
                 "\r\nSec-WebSocket-Version: 13\r\n\r\n";
    c->print(req);
    // Status line, then headers up to the blank line.
    String status = c->readStringUntil('\n');
    if (status.indexOf(" 101") < 0) {
        setErr("socket refused: " + status.substring(0, 40));
        c->stop();
        delete c;
        return false;
    }
    for (int i = 0; i < 40; i++) {
        String line = c->readStringUntil('\n');
        if (line.length() <= 1) break;
    }
    sock = c;
    return true;
}

void wsClose() {
    if (!sock) return;
    sock->stop();
    delete sock;
    sock = nullptr;
}

// ---- Phoenix (Supabase Realtime) ----------------------------------------------------

bool push(const char *topicName, const char *event, JsonDocument &payload) {
    JsonDocument msg;
    msg["topic"] = topicName;
    msg["event"] = event;
    msg["payload"] = payload;
    msg["ref"] = String(++refNo);
    String s;
    serializeJson(msg, s);
    return wsSend(0x1, s);
}

bool broadcast(const char *event, const String &line) {
    JsonDocument p;
    p["type"] = "broadcast";
    p["event"] = event;
    p["payload"]["line"] = line;
    return push(topic.c_str(), "broadcast", p);
}

struct Token {
    String token, realtimeUrl, apikey, channel;
    uint32_t expiresAt = 0; // epoch s
    uint32_t mintedMs = 0;
    uint32_t lifeS = 0; // expiresAt - the server's now, roughly: the board's clock may be unset
};

// POST /api/v1/devices/realtime-token. False = try again later; sets st = REFUSED
// when the site said no for a reason that won't fix itself.
bool mint(Token &t) {
    freq::Settings &s = freq::settings();
    if (s.deviceId.isEmpty() || s.deviceSecret.isEmpty()) {
        setErr("not set up as a scanner yet");
        return false;
    }
    String base = s.baseUrl;
    while (base.endsWith("/")) base.remove(base.length() - 1);
    WiFiClientSecure tls;
    tls.setInsecure();
    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(tls, base + "/api/v1/devices/realtime-token")) {
        setErr("bad base URL");
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Frequency-Device", s.deviceId + "." + s.deviceSecret);
    int code = http.POST("{}");
    JsonDocument body;
    if (code > 0) deserializeJson(body, http.getString());
    http.end();
    String reason = body["reason"] | "";
    if (code == 404 || reason == "relay_disabled") {
        st = REFUSED;
        setErr(code == 404 ? "internet relay is off on the site" : "the owner hasn't switched the relay on");
        return false;
    }
    if (code != 200 || !(body["ok"] | false)) {
        setErr(code < 0 ? HTTPClient::errorToString(code) : "token: " + (reason.length() ? reason : String(code)));
        return false;
    }
    t.token = body["token"] | "";
    t.realtimeUrl = body["realtime_url"] | "";
    t.apikey = body["apikey"] | "";
    t.channel = body["channel"] | "";
    t.expiresAt = body["expires_at"] | 0;
    t.mintedMs = millis();
    t.lifeS = 15 * 60; // the token's lifetime (lib/devices/relay.ts); expires_at is informative
    {
        Guard g;
        allowed.clear();
        for (JsonVariant v : body["allowed_cmds"].as<JsonArray>()) allowed.push_back(v.as<String>());
    }
    return t.token.length() && t.realtimeUrl.length() && t.channel.length();
}

bool allowedCmd(const String &cmd) {
    if (cmd == "cli.run") return false; // never a remote shell over the internet
    Guard g;
    for (const String &a : allowed)
        if (a == cmd) return true;
    return false;
}

// A broadcast arrived on the channel. Only "rpc" lines are run.
void onMessage(const String &text) {
    JsonDocument msg;
    if (deserializeJson(msg, text)) return;
    String event = msg["event"] | "";
    if (event == "phx_reply") {
        String status = msg["payload"]["status"] | "";
        if (status != "ok" && String(msg["topic"] | "") == topic) {
            setErr("join refused: " + String(msg["payload"]["response"]["reason"] | status.c_str()));
            wsClose();
        }
        return;
    }
    if (event == "phx_error" || event == "phx_close") {
        wsClose();
        return;
    }
    if (event != "broadcast") return;
    if (String(msg["payload"]["event"] | "") != "rpc") return;
    String line = msg["payload"]["payload"]["line"] | "";
    line.trim();
    if (!remote::isRpcLine(line)) return; // the relay never takes raw CLI lines
    broadcast("reply", "@rpc " + remote::runLine(line, "net", allowedCmd));
}

bool join(const Token &t) {
    topic = "realtime:" + t.channel;
    JsonDocument p;
    p["config"]["broadcast"]["self"] = false;
    p["config"]["broadcast"]["ack"] = false;
    p["config"]["presence"]["key"] = freq::settings().deviceId;
    p["config"]["private"] = true;
    p["access_token"] = t.token;
    if (!push(topic.c_str(), "phx_join", p)) return false;
    // Presence: the website shows online/offline without polling.
    JsonDocument pr;
    pr["type"] = "presence";
    pr["event"] = "track";
    pr["payload"]["online"] = true;
    pr["payload"]["version"] = OS_VERSION;
    return push(topic.c_str(), "presence", pr);
}

bool ready() {
    if (WiFi.status() != WL_CONNECTED) return setErr("no Wi-Fi"), false;
    if (remote::bleEnabled()) return setErr("Bluetooth is on (not enough RAM for both)"), false;
    if (millis() < yieldUntil) return false;
    if (ESP.getFreeHeap() < MIN_HEAP) return setErr("not enough free RAM"), false;
    return true;
}

void loop(void *) {
    uint32_t backoff = 5000;
    Token tok;
    while (wantOn) {
        if (st == REFUSED) { // only a restart of the relay (off/on) or a re-provision retries quickly
            vTaskDelay(pdMS_TO_TICKS(300000));
            st = WAITING;
            continue;
        }
        if (!ready()) {
            st = WAITING;
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        st = CONNECTING;
        if (!mint(tok) || !ready() || !wsOpen(tok.realtimeUrl, tok.apikey) || !join(tok)) {
            wsClose();
            if (st != REFUSED) st = WAITING;
            vTaskDelay(pdMS_TO_TICKS(backoff));
            backoff = min<uint32_t>(backoff * 2, 60000);
            continue;
        }
        backoff = 5000;
        st = JOINED;
        setErr("");
        uint32_t beat = millis();
        while (wantOn && sock && sock->connected() && WiFi.status() == WL_CONNECTED && !remote::bleEnabled() &&
               millis() >= yieldUntil) {
            String text;
            int r = wsRead(text);
            if (r < 0) break;
            if (r > 0) onMessage(text);
            if (millis() - beat > HEARTBEAT_MS) {
                JsonDocument empty;
                empty.to<JsonObject>();
                if (!push("phoenix", "heartbeat", empty)) break;
                beat = millis();
            }
            if ((millis() - tok.mintedMs) / 1000 > tok.lifeS - REMINT_EARLY_S) {
                Token fresh;
                if (mint(fresh)) {
                    JsonDocument p;
                    p["access_token"] = fresh.token;
                    push(topic.c_str(), "access_token", p);
                    tok = fresh;
                } else if (st == REFUSED) break;
                else tok.mintedMs = millis() - (tok.lifeS - REMINT_EARLY_S - 30) * 1000; // retry in 30 s
            }
            std::vector<String> evts;
            {
                Guard g;
                evts.swap(pendingEvents);
            }
            for (const String &e : evts) broadcast("evt", "@evt " + e);
            if (yieldReq) break;
            if (r == 0) vTaskDelay(pdMS_TO_TICKS(20));
        }
        if (yieldReq) {
            yieldReq = false;
            yieldUntil = millis() + YIELD_MS;
        }
        wsClose();
        if (st == JOINED) st = WAITING;
    }
    wsClose();
    st = OFF;
    task = nullptr;
    vTaskDelete(nullptr);
}

void start() {
    if (task) return;
    st = WAITING;
    // 8 KB like the CLI task: relayed handlers run here.
    xTaskCreatePinnedToCore(loop, "cu_relay", 8192, nullptr, 1, &task, 1);
}

} // namespace

bool enabled() {
    Preferences p;
    p.begin("remote", true);
    bool on = p.getBool("relay", false);
    p.end();
    return on;
}

void setEnabled(bool on) {
    Preferences p;
    p.begin("remote", false);
    p.putBool("relay", on);
    p.end();
    wantOn = on;
    if (on) {
        if (st == REFUSED) st = WAITING;
        start();
    }
}

State state() { return st; }

const char *stateText(State s) {
    switch (s) {
        case OFF: return "off";
        case WAITING: return "waiting";
        case CONNECTING: return "connecting";
        case JOINED: return "online";
        case REFUSED: return "refused";
    }
    return "?";
}

String lastError() {
    Guard g;
    return err;
}

void sendEvent(const String &json) {
    if (st != JOINED) return;
    Guard g;
    if (pendingEvents.size() < 8) pendingEvents.push_back(json); // a burst past 8 is dropped
}

void yieldTls() {
    if (st != JOINED || ESP.getFreeHeap() >= MIN_HEAP) return;
    yieldReq = true; // the relay task closes the socket and stays off for YIELD_MS
    for (int i = 0; i < 60 && st == JOINED; i++) delay(50);
}

void boot() {
    if (enabled()) setEnabled(true);
}

} // namespace relay
