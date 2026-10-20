// Remote commands that set the board up as a Space's scanner over the cable
// (plan Phase 3), and the answer half of scans through the cable (Phase 5).
// Contract: apps/remote/PROTOCOL.md "Setup over the cable". These run on the CLI
// task: no drawing.
#include "../remote/remote.h"
#include "backend.h"
#include "store.h"

namespace {

bool isUuid(const String &s) {
    if (s.length() != 36) return false;
    for (int i = 0; i < 36; i++) {
        bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? s[i] != '-' : !isxdigit((unsigned char)s[i])) return false;
    }
    return true;
}

// frequency-web secrets are 32 random bytes in base64url (43 characters).
bool isSecret(const String &s) {
    if (s.length() < 32 || s.length() > 128) return false;
    for (char c : s)
        if (!isalnum((unsigned char)c) && c != '-' && c != '_') return false;
    return true;
}

// What the board is set up as. The secret itself never comes back.
void putConfig(JsonObject o) {
    freq::Settings &s = freq::settings();
    o["reader"] = s.readerMode;
    o["base"] = s.baseUrl;
    o["device_id"] = s.deviceId;
    o["secret_set"] = s.deviceSecret.length() > 0;
    o["ready"] = s.readerReady();
    o["queued"] = (int)freq::queuedScans().size();
    o["clock_set"] = remote::trustedEpoch() != 0;
    o["cable"] = freq::cableAvailable();
}

void cmdConfig(remote::Request &r) { putConfig(r.data); }

// frequency.provision {device_id, secret, base?, reader?: true}
void cmdProvision(remote::Request &r) {
    String id = r.args["device_id"] | "";
    String secret = r.args["secret"] | "";
    String base = r.args["base"] | "";
    id.trim();
    id.toLowerCase();
    if (!isUuid(id)) return r.fail("bad_args", "device_id must be the scanner's uuid");
    if (!isSecret(secret)) return r.fail("bad_args", "secret must be the scanner's base64url secret");
    if (base.length() && !base.startsWith("https://") && !base.startsWith("http://"))
        return r.fail("bad_args", "base must start with https://");

    freq::Settings &s = freq::settings();
    bool changedDevice = s.deviceId.length() && s.deviceId != id;
    s.deviceId = id;
    s.deviceSecret = secret;
    if (base.length()) {
        while (base.endsWith("/")) base.remove(base.length() - 1);
        s.baseUrl = base;
    }
    s.readerMode = r.args["reader"] | true;
    s.save();
    // Scans queued under another scanner's key would only be refused.
    if (changedDevice) freq::removeQueued(freq::queuedScans());
    putConfig(r.data);
}

// frequency.unprovision {}: forget the scanner's key and the queue (the website
// revoked it). Venue tags still write: they need no key.
void cmdUnprovision(remote::Request &r) {
    freq::Settings &s = freq::settings();
    s.deviceId = "";
    s.deviceSecret = "";
    s.readerMode = false;
    s.save();
    freq::removeQueued(freq::queuedScans());
    putConfig(r.data);
}

// frequency.relay_reply {nonce, status, body}: the server's answer to a
// frequency.scan_relay event. status is the HTTP status, or 0 when the computer
// couldn't reach the server either (the board then queues the scan).
void cmdRelayReply(remote::Request &r) {
    String nonceHex = r.args["nonce"] | "";
    int status = r.args["status"] | -1;
    if (nonceHex.length() != 8 || status < 0 || status > 599)
        return r.fail("bad_args", "nonce (8 hex) and status (0-599) are required");
    uint32_t nonce = strtoul(nonceHex.c_str(), nullptr, 16);
    String body;
    serializeJson(r.args["body"], body);
    if (!freq::relayAnswer(nonce, status, body)) return r.fail("not_waiting", "no scan is waiting on that nonce");
    r.data["nonce"] = nonceHex;
}

} // namespace

REMOTE_COMMAND("frequency.config", cmdConfig);
REMOTE_COMMAND("frequency.provision", cmdProvision);
REMOTE_COMMAND("frequency.unprovision", cmdUnprovision);
REMOTE_COMMAND("frequency.relay_reply", cmdRelayReply);
