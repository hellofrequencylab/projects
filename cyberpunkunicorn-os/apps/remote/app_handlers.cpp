// Remote commands for the apps that landed on other branches (Sound, Light,
// Sensors, Frequency), wired in at merge time. Contract: apps/remote/PROTOCOL.md.
// These run on the CLI task: no drawing, and every blocking wait is bounded.
#include "remote.h"

#include "../cyd_pins.h"
#include "../frequency/backend.h"
#include "../frequency/card.h"
#include "../frequency/store.h"
#include "../sensors/sensor_hub.h"

namespace {

// ---- sound.tone {hz, ms} ---------------------------------------------------------

void cmdSoundTone(remote::Request &r) {
    int hz = r.args["hz"] | 440, ms = r.args["ms"] | 300;
    if (hz < 20 || hz > 20000) return r.fail("bad_args", "hz is 20-20000");
    if (ms < 1 || ms > 5000) return r.fail("bad_args", "ms is 1-5000");
    tone(CYD_SPEAKER, hz, ms);
    r.data["hz"] = hz;
    r.data["ms"] = ms;
}

// ---- light.read ------------------------------------------------------------------

void cmdLightRead(remote::Request &r) {
    long sum = 0;
    for (int i = 0; i < 8; i++) sum += analogRead(CYD_LDR);
    int raw = sum / 8;
    int pct = raw * 100 / 4095;
    if (CYD_LDR_BRIGHT_LOW) pct = 100 - pct;
    r.data["raw"] = raw;
    r.data["percent"] = pct;
}

// ---- sensors.read ----------------------------------------------------------------

void cmdSensorsRead(remote::Request &r) {
    // Borrow a short wired-only session if the Sensors app isn't already running one.
    bool borrowed = !sensors::sessionActive();
    if (borrowed) {
        sensors::SessionInfo info = sensors::beginSession(false);
        if (!info.wiredOk) {
            sensors::endSession();
            return r.fail("busy", info.warning.length() ? info.warning : String("wired sensors unavailable"));
        }
        for (int i = 0; i < 6; i++) { // two-step chips need a couple of polls
            sensors::poll();
            delay(60);
        }
    } else sensors::poll();

    JsonArray list = r.data["channels"].to<JsonArray>();
    sensors::Channel ch;
    for (int i = 0; i < sensors::channelCount(); i++) {
        if (!sensors::channel(i, ch)) continue;
        JsonObject o = list.add<JsonObject>();
        o["source"] = ch.source;
        o["label"] = ch.label;
        o["value"] = ch.value;
        o["unit"] = sensors::unitText(ch.unit);
        o["type"] = sensors::typeText(ch.type);
        o["wireless"] = ch.wireless;
        o["age_ms"] = millis() - ch.updatedMs;
    }
    if (borrowed) sensors::endSession();
}

// ---- frequency.* -----------------------------------------------------------------

constexpr uint32_t TAP_TIMEOUT_MS = 10000;

void putCard(JsonObject o, const freq::CardInfo &c) {
    o["uid"] = c.uid;
    o["type"] = c.type;
    o["ntag"] = c.ntag;
    o["locked"] = c.locked;
    o["capacity"] = c.capacity;
    o["uri"] = c.uri;
    freq::UrlCheck u = freq::checkUrl(c.uri);
    o["kind"] = freq::kindName(u.kind);
}

// Opens the reader and waits (bounded) for a tag. Fails the request on any error.
bool tap(remote::Request &r, freq::CardInfo &card) {
    freq::CardStatus s = freq::readerOpen();
    if (s != freq::CARD_OK) {
        r.fail(s == freq::CARD_NO_READER ? "no_reader" : "busy", freq::cardStatusText(s));
        return false;
    }
    uint32_t timeout = r.args["timeout_ms"] | TAP_TIMEOUT_MS;
    if (timeout > 30000) timeout = 30000;
    remote::emit("frequency.waiting_for_tag", JsonObjectConst());
    s = freq::waitCard(card, timeout, [] { return false; });
    if (s != freq::CARD_OK) {
        freq::readerClose();
        r.fail(s == freq::CARD_NONE ? "timeout" : "card_error", freq::cardStatusText(s));
        return false;
    }
    return true;
}

void cmdReadTag(remote::Request &r) {
    freq::CardInfo card;
    if (!tap(r, card)) return;
    freq::readerClose();
    putCard(r.data, card);
}

void cmdWriteTag(remote::Request &r) {
    String url = r.args["url"] | "";
    String label = r.args["label"] | "";
    freq::UrlCheck u = freq::checkUrl(url);
    if (!u.ok) return r.fail("bad_url", u.problem.length() ? u.problem : String("not a Frequency URL"));
    freq::CardInfo card;
    if (!tap(r, card)) return;
    freq::CardStatus s = freq::writeCard(u.fixed, card);
    freq::readerClose();
    if (s != freq::CARD_OK) return r.fail("write_failed", freq::cardStatusText(s));
    freq::logWrite(label, u, card.uid, false);
    putCard(r.data, card);
    r.data["written"] = u.host + u.path; // no query: the signature stays out of logs
}

// Member-card check-in through frequency-web's device API (BACKEND.md).
void cmdScan(remote::Request &r) {
    freq::CardInfo card;
    if (!tap(r, card)) return;
    freq::readerClose();
    putCard(r.data["card"].to<JsonObject>(), card);
    freq::UrlCheck u = freq::checkUrl(card.uri);
    if (u.kind != freq::URL_MEMBER) return r.fail("not_member_card", "that tag isn't a Frequency member card");
    // No Wi-Fi, but the caller is the computer on the cable: hand it the signed
    // scan to forward. This handler can't wait for frequency.relay_reply (it runs
    // on the task that would deliver it), so the computer posts it and keeps the answer.
    if (freq::cableAvailable()) {
        freq::SignedScan sc = freq::signScan(u.token, card.uid, remote::trustedEpoch());
        freq::putRelay(r.data["relay"].to<JsonObject>(), sc);
        return;
    }
    freq::ScanReply s = freq::postScan(u.token, card.uid);
    switch (s.status) {
        case freq::API_OK: break;
        case freq::API_DISABLED: return r.fail("reader_mode_off", "turn on reader mode and set the device id/secret");
        case freq::API_NOT_SET_UP: return r.fail("backend_not_set_up", freq::reasonWords("not_enabled").hint);
        case freq::API_OFFLINE:
            if (!freq::queueScan({freq::nowEpoch(), card.uid, u.token})) return r.fail("offline", "queue full");
            return r.fail("offline", "queued; will retry when online");
        default: return r.fail("rejected", s.reason);
    }
    r.data["member"]["display_name"] = s.name;
    r.data["zapsAwarded"] = s.zaps;
    if (s.hasLoyalty) {
        JsonObject l = r.data["loyalty"].to<JsonObject>();
        l["stamps"] = s.stamps;
        l["required"] = s.required;
        l["earned"] = s.earned;
        if (s.rewardCode.length()) l["reward_code"] = s.rewardCode;
    }
}

} // namespace

REMOTE_COMMAND("sound.tone", cmdSoundTone);
REMOTE_COMMAND("light.read", cmdLightRead);
REMOTE_COMMAND("sensors.read", cmdSensorsRead);
REMOTE_COMMAND("frequency.read_tag", cmdReadTag);
REMOTE_COMMAND("frequency.write_tag", cmdWriteTag);
REMOTE_COMMAND("frequency.scan", cmdScan);
