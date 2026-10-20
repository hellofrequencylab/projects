// `frequency` serial command: everything the screen does, testable without it.
//   frequency status                 reader, settings, queue
//   frequency read [timeout_ms]      what the next tag points to
//   frequency tag <url> [lock-forever]   write (and optionally lock) a tag
//   frequency tags                   the tags.txt pick list, checked
//   frequency log                    tags written
//   frequency scan [timeout_ms]      member-card scan (reader mode)
//   frequency issue <handle> [timeout_ms]  issue + write a member card
//   frequency queue [retry]          offline scan queue
//   frequency config [key [value]]   reader|base|devid|devsecret
#include "FrequencyMenu.h"
#include "backend.h"
#include "card.h"
#include "store.h"
#include <globals.h>

using namespace freq;

namespace {

void out(const String &s) { serialDevice->println(s); }

uint32_t argTimeout(Command &cmd, int index, uint32_t fallback = 10000) {
    if (cmd.countArgs() <= index) return fallback;
    uint32_t t = cmd.getArgument(index).getValue().toInt();
    return t ? t : fallback;
}

bool claim() {
    CardStatus s = readerOpen();
    if (s == CARD_OK) return true;
    out(String("ERROR: ") + cardStatusText(s));
    return false;
}

void printTag(const CardInfo &c) {
    UrlCheck u = checkUrl(c.uri);
    out("UID:   " + c.uid);
    out("Type:  " + c.type + (c.locked ? " (locked)" : ""));
    if (c.ntag) out("Space: " + String(c.capacity) + " bytes NDEF");
    out("URL:   " + (c.uri.length() ? c.uri : String("(blank)")));
    out(String("Kind:  ") + kindName(u.kind) + (u.ok ? "" : (u.problem.length() ? " - " + u.problem : "")));
}

bool waitTag(CardInfo &c, uint32_t timeout) {
    out("Tap a tag (" + String(timeout) + " ms)...");
    if (waitCard(c, timeout, nullptr) == CARD_OK) return true;
    out("No tag.");
    return false;
}

uint32_t cbStatus(cmd *) {
    storeBegin();
    Settings &s = settings();
    CardStatus r = readerOpen();
    out(String("Reader:      ") + (r == CARD_OK ? "PN532 ready" : cardStatusText(r)));
    if (r == CARD_OK) readerClose();
    out(String("Reader mode: ") + (s.readerMode ? "ON (proposed API, see BACKEND.md)" : "OFF"));
    out("Base URL:    " + s.baseUrl);
    out("Device ID:   " + (s.deviceId.length() ? s.deviceId : String("(unset)")));
    out(String("Secret:      ") + (s.deviceSecret.length() ? "set" : "(unset)"));
    out("Queue:       " + String(queuedScans().size()) + " scan(s)");
    return true;
}

uint32_t cbRead(cmd *c) {
    Command cmd(c);
    if (!claim()) return false;
    CardInfo t;
    bool ok = waitTag(t, argTimeout(cmd, 0));
    if (ok) printTag(t);
    readerClose();
    return ok;
}

uint32_t cbTag(cmd *c) {
    Command cmd(c);
    if (cmd.countArgs() < 1) {
        out("Usage: frequency tag <url> [lock-forever]");
        return false;
    }
    String url = cmd.getArgument(0).getValue();
    bool lock = cmd.countArgs() > 1 && cmd.getArgument(1).getValue() == "lock-forever";
    UrlCheck u = checkUrl(url);
    out(String("Kind: ") + kindName(u.kind));
    if (!u.ok) out("WARNING: " + u.problem + " (writing anyway)");
    if (u.fixed != url) out("Writing as: " + u.fixed);
    storeBegin();
    if (!claim()) return false;
    CardInfo t, done;
    bool ok = waitTag(t, 15000);
    if (ok) {
        CardStatus s = writeCard(u.fixed, done);
        ok = s == CARD_OK;
        out(String("Write: ") + cardStatusText(s));
        if (ok && lock) {
            CardStatus ls = lockCard(done.uid);
            out(String("Lock:  ") + (ls == CARD_OK ? "LOCKED FOREVER" : cardStatusText(ls)));
            done.locked = ls == CARD_OK;
        }
        if (ok) {
            printTag(done);
            logWrite("serial", checkUrl(done.uri), done.uid, done.locked);
        }
    }
    readerClose();
    return ok;
}

uint32_t cbTags(cmd *) {
    storeBegin();
    String from;
    std::vector<TagSource> list = loadTagList(from);
    if (list.empty()) {
        out(String("No ") + TAG_LIST_PATH + " on SD or LittleFS. One 'label|url' per line.");
        return true;
    }
    out(String(list.size()) + " tag(s) from " + from + ":");
    for (const TagSource &t : list) {
        UrlCheck u = checkUrl(t.url);
        out("  " + t.label + "  [" + kindName(u.kind) + (u.ok ? "" : ": " + u.problem) + "]");
        out("    " + u.host + u.path);
    }
    return true;
}

uint32_t cbLog(cmd *) {
    storeBegin();
    out("ts,uid,kind,label,url,locked");
    for (const String &l : recentWrites(100)) out(l);
    return true;
}

uint32_t cbScan(cmd *c) {
    Command cmd(c);
    storeBegin();
    if (!settings().readerReady()) {
        out("Reader mode is off or device id/secret unset (frequency config).");
        out("Note: the device API is a proposal; frequency-web doesn't serve it yet.");
        return false;
    }
    if (!claim()) return false;
    CardInfo t;
    bool ok = waitTag(t, argTimeout(cmd, 0));
    readerClose();
    if (!ok) return false;
    UrlCheck u = checkUrl(t.uri);
    if (u.kind != URL_MEMBER || !u.ok) {
        out("Not a member card:");
        printTag(t);
        return false;
    }
    ScanReply r = postScan(u.token, t.uid);
    switch (r.status) {
        case API_OK:
            out("Checked in: " + r.name + "  +" + String(r.zaps) + " zaps");
            if (r.hasLoyalty)
                out("Stamps: " + String(r.stamps) + "/" + String(r.required) + (r.earned ? "  REWARD " + r.rewardCode : ""));
            return true;
        case API_OFFLINE:
            out("Offline (" + r.reason + "): " + (queueScan({nowEpoch(), t.uid, u.token}) ? "queued" : "queue full"));
            return false;
        default: out("ERROR: " + r.reason); return false;
    }
}

uint32_t cbIssue(cmd *c) {
    Command cmd(c);
    if (cmd.countArgs() < 1) {
        out("Usage: frequency issue <member_handle> [timeout_ms]");
        return false;
    }
    IssueReply r = issueCard(cmd.getArgument(0).getValue());
    if (r.status != API_OK) {
        out("ERROR: " + r.reason);
        return false;
    }
    out("Card URL: " + r.url);
    if (!claim()) return false;
    CardInfo t, done;
    bool ok = waitTag(t, argTimeout(cmd, 1, 15000));
    if (ok) {
        CardStatus s = writeCard(r.url, done);
        ok = s == CARD_OK;
        out(String("Write: ") + cardStatusText(s));
        if (ok) printTag(done);
    }
    readerClose();
    return ok;
}

uint32_t cbQueue(cmd *c) {
    Command cmd(c);
    storeBegin();
    std::vector<QueuedScan> q = queuedScans();
    out(String(q.size()) + " queued scan(s)");
    for (const QueuedScan &s : q) out("  " + String(s.ts) + "  " + s.uid);
    if (cmd.countArgs() > 0 && cmd.getArgument(0).getValue() == "retry") {
        String err;
        int n = retryQueue(err);
        out(String(n) + " answered" + (err.length() ? ", stopped: " + err : ""));
    }
    return true;
}

uint32_t cbConfig(cmd *c) {
    Command cmd(c);
    Settings &s = settings();
    if (cmd.countArgs() < 2) {
        out(String("reader    ") + (s.readerMode ? "on" : "off"));
        out("base      " + s.baseUrl);
        out("devid     " + s.deviceId);
        out(String("devsecret ") + (s.deviceSecret.length() ? "(set)" : "(unset)"));
        out("Set: frequency config <key> <value>   (value '-' clears)");
        return true;
    }
    String key = cmd.getArgument(0).getValue(), val = cmd.getArgument(1).getValue();
    if (val == "-") val = "";
    if (key == "reader") s.readerMode = val == "on" || val == "1" || val == "true";
    else if (key == "base") s.baseUrl = val.length() ? val : String(DEFAULT_BASE_URL);
    else if (key == "devid") s.deviceId = val;
    else if (key == "devsecret") s.deviceSecret = val;
    else {
        out("Unknown key. Keys: reader base devid devsecret");
        return false;
    }
    s.save();
    out("Saved " + key);
    return true;
}

} // namespace

void createFrequencyCommand(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCmd("frequency,freq");
    cmd.addCommand("status", cbStatus);
    cmd.addBoundlessCommand("read", cbRead);
    cmd.addBoundlessCommand("tag", cbTag);
    cmd.addCommand("tags", cbTags);
    cmd.addCommand("log", cbLog);
    cmd.addBoundlessCommand("scan", cbScan);
    cmd.addBoundlessCommand("issue", cbIssue);
    cmd.addBoundlessCommand("queue", cbQueue);
    cmd.addBoundlessCommand("config", cbConfig);
}
