#include "FrequencyMenu.h"

#include "../ui/rainbow.h"
#include "backend.h"
#include "card.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "fx.h"
#include "store.h"
#include <globals.h>

using namespace freq;

namespace {

constexpr int TOP = 46; // below the skinned title
const char *ESC_KEY = "\x1B";

// ---- small screen helpers ------------------------------------------------------

// A pick list. Returns the chosen index, or -1 on ESC.
int choose(const String &title, const std::vector<String> &labels) {
    int picked = -1;
    std::vector<Option> opts;
    for (size_t i = 0; i < labels.size(); i++)
        opts.push_back({labels[i].c_str(), [&picked, i]() { picked = i; }});
    loopOptions(opts, MENU_TYPE_SUBMENU, title.c_str());
    return picked;
}

void waitAnyKey(uint32_t timeoutMs = 0) {
    uint32_t start = millis();
    delay(150);
    check(AnyKeyPress);
    while (!check(AnyKeyPress)) {
        if (timeoutMs && millis() - start > timeoutMs) return;
        delay(20);
    }
}

void line(int y, const String &label, const String &value, uint16_t color = TFT_WHITE) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(label, 14, y, 1);
    tft.setTextColor(color, bruceConfig.bgColor);
    String v = value;
    int room = (tftWidth - 90) / LW;
    if ((int)v.length() > room) v = v.substring(0, room - 2) + "..";
    tft.drawString(v, 82, y, 1);
}

uint16_t kindColor(UrlKind k) {
    switch (k) {
        case URL_VENUE: return cyberHue(160);
        case URL_EVENT: return cyberHue(45);
        case URL_MEMBER: return cyberHue(300);
        default: return TFT_LIGHTGREY;
    }
}

void footer(const String &text) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString(text, tftWidth / 2, tftHeight - 34, 1);
}

// "DIP 3 is off" screen with the switch drawn, number 3 lit.
void showDipThree() {
    drawMainBorderWithTitle("FREQUENCY");
    fxCentreText("TURN DIP 3 ON", TOP + 20, 2, 0);
    int sw = 22, gap = 10, x0 = (tftWidth - (6 * sw + 5 * gap)) / 2, y0 = TOP + 50;
    for (int i = 0; i < 6; i++) {
        int x = x0 + i * (sw + gap);
        bool on = i == 2;
        uint16_t c = on ? cyberHue(300) : TFT_DARKGREY;
        tft.drawRect(x, y0, sw, 44, c);
        tft.fillRect(x + 2, on ? y0 + 2 : y0 + 22, sw - 4, 20, c);
        tft.setTextColor(c, bruceConfig.bgColor);
        tft.drawCentreString(String(i + 1), x + sw / 2, y0 + 50, 1);
    }
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawCentreString("The PN532 (NFC) is not answering.", tftWidth / 2, y0 + 70, 1);
    tft.drawCentreString("Only one of DIP 1-5 may be ON.", tftWidth / 2, y0 + 82, 1);
    footer("any key: back");
    waitAnyKey();
}

// Claims the reader for this screen, or explains why not.
bool openReader() {
    displayInfo("Waking the PN532...");
    CardStatus s = readerOpen();
    if (s == CARD_OK) return true;
    if (s == CARD_NO_READER) showDipThree();
    else displayError(cardStatusText(s), true);
    return false;
}

// Waits for a tap with the rainbow wave running. False = ESC.
bool tapScreen(const String &title, const String &prompt, const String &detail, CardInfo &out) {
    drawMainBorderWithTitle(title);
    fxCentreText(prompt, TOP + 10, 2, 0);
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, bruceConfig.bgColor);
    String d = detail;
    if ((int)d.length() * LW > tftWidth - 20) d = d.substring(0, (tftWidth - 20) / LW - 2) + "..";
    tft.drawCentreString(d, tftWidth / 2, TOP + 26, 1);
    footer("ESC: back");
    int t = 0;
    const int wy = TOP + 42, wh = tftHeight - wy - 42;
    CardStatus s = waitCard(
        out, 0, []() { return check(EscPress); }, [&]() { fxWave(12, wy, tftWidth - 24, wh, t++); }
    );
    return s == CARD_OK;
}

// Holds until the tag `uid` has left the field, so a loop doesn't instantly
// re-read (or re-write) the tag that is still lying on the reader.
void waitRemoved(const String &uid) {
    displayInfo("Lift the tag off...");
    CardInfo c;
    uint32_t start = millis();
    for (int misses = 0; misses < 2 && millis() - start < 20000;) {
        if (pollCard(c) == CARD_OK && c.uid == uid) misses = 0;
        else misses++;
        if (check(EscPress)) return;
        delay(60);
    }
}

// What a tag holds, for Verify and after writing.
void showTag(const CardInfo &c, const String &title) {
    UrlCheck u = checkUrl(c.uri);
    drawMainBorderWithTitle(title);
    String badge = c.uri.isEmpty() ? "BLANK TAG" : String(kindName(u.kind));
    badge.toUpperCase();
    tft.setTextSize(2);
    tft.setTextColor(kindColor(u.kind), bruceConfig.bgColor);
    tft.drawCentreString(badge, tftWidth / 2, TOP, 1);
    int y = TOP + 26;
    line(y, "Tag", c.type + (c.locked ? "  LOCKED" : ""));
    line(y += 12, "UID", c.uid);
    if (c.uri.length()) {
        line(y += 12, "Host", u.host, cyberHue(180));
        line(y += 12, "Path", u.path, cyberHue(200));
        if (u.kind == URL_VENUE) line(y += 12, "Signed", "yes (?s=v1...)", cyberHue(120));
        if (u.kind == URL_EVENT) line(y += 12, "NFC mark", u.fixed == c.uri ? "?m=nfc ok" : "missing ?m=nfc");
        if (!u.ok) line(y += 12, "Problem", u.problem, TFT_ORANGE);
    }
    if (c.ntag) line(y += 12, "Space", String(c.capacity) + " bytes NDEF");
}

// ---- Program venue tag ----------------------------------------------------------

bool pickUrl(String &label, String &url) {
    String from;
    std::vector<TagSource> list = loadTagList(from);
    std::vector<String> labels;
    for (const TagSource &t : list) labels.push_back(t.label);
    labels.push_back("Type a URL...");
    labels.push_back("Paste over serial (help)");
    int i = choose(list.empty() ? String("No tags.txt yet") : "From " + from + " tags.txt", labels);
    if (i < 0) return false;
    if (i < (int)list.size()) {
        label = list[i].label;
        url = list[i].url;
        return true;
    }
    if (i == (int)list.size() + 1) {
        ScrollableTextArea area("SERIAL / FILE");
        area.addLine("Serial (115200):");
        area.addLine("  frequency tag <url>");
        area.addLine("");
        area.addLine("Or a pick list, one per line:");
        area.addLine("  label|url");
        area.addLine("in " + String(TAG_LIST_PATH));
        area.addLine("on SD or LittleFS (upload it with");
        area.addLine("the web UI file manager).");
        area.addLine("");
        area.addLine("Venue URL: Space settings >");
        area.addLine("Check-in > copy link.");
        area.show();
        return false;
    }
    url = keyboard("https://frequencylocal.com/", 200, "Check-in or event URL");
    if (url == ESC_KEY || url.isEmpty()) return false;
    label = "";
    return true;
}

// Confirms a lock twice: it can never be undone.
bool confirmLock() {
    drawMainBorderWithTitle("LOCK TAG?");
    tft.setTextSize(3);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("PERMANENT", tftWidth / 2, TOP + 6, 1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawCentreString("Locking makes this tag read-only", tftWidth / 2, TOP + 44, 1);
    tft.drawCentreString("FOREVER. If the venue link changes,", tftWidth / 2, TOP + 56, 1);
    tft.drawCentreString("the tag goes in the bin.", tftWidth / 2, TOP + 68, 1);
    footer("any key: continue");
    waitAnyKey();
    return choose("REALLY LOCK?", {"No, keep it writable", "Yes, lock it forever"}) == 1;
}

void celebrateWrite(const CardInfo &c, const String &label) {
    fxBurst(tftWidth / 2, tftHeight / 2);
    drawMainBorderWithTitle("TAG WRITTEN");
    for (int f = 0; f < 6; f++) {
        fxUnicorn(tftWidth / 2 - UNICORN_W, TOP, 2, f & 1, f * 30);
        delay(90);
    }
    fxCentreText(label.length() ? label : String("NAILED IT"), TOP + 70, 2, 40);
    UrlCheck u = checkUrl(c.uri);
    line(TOP + 92, "Tag", c.type + "  " + c.uid);
    line(TOP + 104, "Points to", u.host + u.path, kindColor(u.kind));
    line(TOP + 116, "Verified", "read back OK", cyberHue(120));
}

void programTag() {
    String label, url;
    if (!pickUrl(label, url)) return;
    UrlCheck u = checkUrl(url);
    if (!u.ok || u.kind == URL_MEMBER) {
        String why = u.kind == URL_MEMBER ? "That is a member card link" : u.problem;
        if (choose("Odd URL: " + why, {"Cancel", "Write it anyway"}) != 1) return;
    }
    if (label.isEmpty()) {
        label = keyboard("", 24, "Label (table, door...)");
        if (label == ESC_KEY) return;
        if (label.isEmpty()) label = kindName(u.kind);
    }
    if (!openReader()) return;

    for (;;) {
        CardInfo c;
        if (!tapScreen("PROGRAM TAG", "TAP A TAG", label + "  " + u.host + u.path, c)) break;
        displayInfo("Writing...");
        CardInfo done;
        CardStatus s = writeCard(u.fixed, done);
        if (s != CARD_OK) {
            displayError(cardStatusText(s), true);
            continue;
        }
        celebrateWrite(done, label);
        footer("any key: next");
        waitAnyKey(6000);
        bool locked = false;
        int next = choose("Tag " + done.uid, {"Write another (same URL)", "Lock this tag...", "Done"});
        if (next == 1 && confirmLock()) {
            displayInfo("Hold the tag still...");
            CardStatus ls = lockCard(done.uid);
            locked = ls == CARD_OK;
            if (locked) displaySuccess("Locked forever", true);
            else displayError(String("Not locked: ") + cardStatusText(ls), true);
            next = choose("Next?", {"Write another (same URL)", "Done"}) == 0 ? 0 : 2;
        }
        logWrite(label, checkUrl(done.uri), done.uid, locked);
        if (next != 0) break;
        waitRemoved(done.uid);
    }
    readerClose();
}

// ---- Verify ------------------------------------------------------------------

void verifyTags() {
    if (!openReader()) return;
    for (;;) {
        CardInfo c;
        if (!tapScreen("VERIFY TAG", "TAP ANY TAG", "shows where a tag points", c)) break;
        showTag(c, "TAG CHECK");
        footer("ESC: back   any other key: next tag");
        delay(200);
        bool esc = false, next = false;
        while (!esc && !next) {
            esc = check(EscPress);
            next = check(SelPress) || check(NextPress) || check(PrevPress);
            delay(20);
        }
        if (esc) break;
        waitRemoved(c.uid);
    }
    readerClose();
}

// ---- Member scan (proposed backend) ---------------------------------------------

void explainReaderMode() {
    ScrollableTextArea area("MEMBER SCAN");
    area.addLine("Reader mode is OFF.");
    area.addLine("");
    area.addLine("It talks to a PROPOSED device API");
    area.addLine("(/api/v1/devices/scan) that");
    area.addLine("frequency-web does NOT have yet.");
    area.addLine("");
    area.addLine("Today members check in with their");
    area.addLine("own phone: tap a venue tag made");
    area.addLine("with 'Program venue tag'.");
    area.addLine("");
    area.addLine("To try it: Settings > Reader mode,");
    area.addLine("device id + secret. See BACKEND.md.");
    area.show();
}

void celebrateScan(const ScanReply &r) {
    fxBurst(tftWidth / 2, tftHeight / 2);
    drawMainBorderWithTitle("CHECKED IN");
    fxCentreText(r.name, TOP + 8, 3, 0);
    if (r.zaps) {
        tft.setTextSize(2);
        tft.setTextColor(cyberHue(55), bruceConfig.bgColor);
        tft.drawCentreString("+" + String(r.zaps) + " ZAPS", tftWidth / 2, TOP + 30, 1);
    }
    if (r.hasLoyalty && r.required > 0) {
        int stamps = constrain(r.stamps, 0, r.required);
        if (r.earned && stamps == 0) stamps = r.required; // card rolled over: show it full
        fxStampCard(10, TOP + 52, tftWidth - 20, r.required, max(0, stamps - 1), stamps);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.drawCentreString(
            String(stamps) + " / " + String(r.required) + " stamps", tftWidth / 2, tftHeight - 48, 1
        );
    }
    if (r.earned) {
        delay(500);
        fxBurst(tftWidth / 2, tftHeight / 2);
        drawMainBorderWithTitle("REWARD!");
        for (int f = 0; f < 10; f++) {
            fxUnicorn(tftWidth / 2 - UNICORN_W * 3 / 2, TOP, 3, f & 1, f * 36);
            delay(80);
        }
        fxCentreText("REWARD EARNED", TOP + 100, 2, 0);
        if (r.rewardCode.length()) fxCentreText(r.rewardCode, TOP + 128, 3, 120);
    }
    footer("any key: next member");
    waitAnyKey(r.earned ? 20000 : 8000);
}

void memberScan() {
    if (!settings().readerReady()) {
        explainReaderMode();
        return;
    }
    if (!openReader()) return;
    for (;;) {
        CardInfo c;
        if (!tapScreen("MEMBER SCAN", "TAP MEMBER CARD", "proposed API - see BACKEND.md", c)) break;
        UrlCheck u = checkUrl(c.uri);
        if (u.kind != URL_MEMBER || !u.ok) {
            showTag(c, "NOT A MEMBER CARD");
            footer("any key: back to scanning");
            waitAnyKey(6000);
            waitRemoved(c.uid);
            continue;
        }
        displayInfo("Checking in...");
        ScanReply r = postScan(u.token, c.uid);
        switch (r.status) {
            case API_OK: {
                celebrateScan(r);
                String err;
                if (queuedScans().size()) retryQueue(err);
                break;
            }
            case API_OFFLINE:
                if (queueScan({nowEpoch(), c.uid, u.token})) displayWarning("Offline: saved, will retry", true);
                else displayError("Offline and the queue is full", true);
                break;
            default: displayError(r.reason, true); break;
        }
        waitRemoved(c.uid);
    }
    readerClose();
}

// ---- Issue member card ------------------------------------------------------------

void issueMemberCard() {
    if (!settings().readerReady()) {
        explainReaderMode();
        return;
    }
    String handle = keyboard("", 40, "Member handle");
    if (handle == ESC_KEY || handle.isEmpty()) return;
    displayInfo("Asking the server for a card...");
    IssueReply r = issueCard(handle);
    if (r.status != API_OK) {
        displayError(r.reason, true);
        return;
    }
    if (!openReader()) return;
    CardInfo c;
    while (tapScreen("ISSUE CARD", "TAP A BLANK CARD", "for " + handle, c)) {
        CardInfo done;
        displayInfo("Writing...");
        CardStatus s = writeCard(r.url, done);
        if (s != CARD_OK) {
            displayError(cardStatusText(s), true);
            continue;
        }
        celebrateWrite(done, handle);
        footer("any key: done");
        waitAnyKey(8000);
        break;
    }
    readerClose();
}

// ---- Logs, queue, settings --------------------------------------------------------

void showWrites() {
    ScrollableTextArea area("TAGS WRITTEN");
    std::vector<String> rows = recentWrites(60);
    if (rows.empty()) area.addLine("No tags written yet.");
    for (int i = rows.size() - 1; i >= 0; i--) { // newest first
        // ts,uid,kind,label,url,locked
        String r = rows[i];
        int a = r.indexOf(','), b = r.indexOf(',', a + 1), c = r.indexOf(',', b + 1), d = r.indexOf(',', c + 1),
            e = r.lastIndexOf(',');
        area.addLine(r.substring(c + 1, d) + (r.endsWith(",1") ? "  [locked]" : ""));
        area.addLine("  " + r.substring(d + 1, e));
        area.addLine("  " + r.substring(b + 1, c) + "  " + r.substring(a + 1, b));
    }
    area.show();
}

void showQueue() {
    std::vector<QueuedScan> q = queuedScans();
    int i = choose(
        String(q.size()) + " scan(s) waiting", {"Retry now", "Back"}
    );
    if (i != 0) return;
    displayInfo("Retrying...");
    String err;
    int n = retryQueue(err);
    if (err.length()) displayWarning(String(n) + " sent, stopped: " + err, true);
    else displaySuccess(String(n) + " sent", true);
}

void editSettings() {
    for (;;) {
        Settings &s = settings();
        int i = choose(
            "SETTINGS",
            {String("Reader mode: ") + (s.readerMode ? "ON (proposed API)" : "OFF"),
             "Base URL: " + s.baseUrl,
             "Device ID: " + (s.deviceId.length() ? s.deviceId : String("(unset)")),
             String("Device secret: ") + (s.deviceSecret.length() ? "set" : "(unset)"),
             "Back"}
        );
        String v;
        switch (i) {
            case 0: s.readerMode = !s.readerMode; break;
            case 1:
                v = keyboard(s.baseUrl, 120, "frequency-web base URL");
                if (v != ESC_KEY && v.length()) s.baseUrl = v;
                break;
            case 2:
                v = keyboard(s.deviceId, 40, "Device ID");
                if (v != ESC_KEY) s.deviceId = v;
                break;
            case 3:
                v = keyboard("", 80, "Device secret", true);
                if (v != ESC_KEY) s.deviceSecret = v;
                break;
            default: return;
        }
        s.save();
    }
}

} // namespace

void FrequencyMenu::optionsMenu() {
    storeBegin();
    size_t queued = queuedScans().size();
    options = {
        {"Program venue tag", []() { programTag(); }     },
        {"Verify tag",        []() { verifyTags(); }     },
        {"Member scan",       []() { memberScan(); }     },
        {"Issue member card", []() { issueMemberCard(); }},
        {"Tags written",      []() { showWrites(); }     },
    };
    if (queued) options.push_back({("Offline queue (" + String(queued) + ")").c_str(), []() { showQueue(); }});
    options.push_back({"Settings", []() { editSettings(); }});
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Frequency");
}

void FrequencyMenu::drawIcon(float scale) {
    clearIconArea();
    // A tag with rainbow NFC waves coming off it.
    int cw = scale * 26, ch = scale * 36;
    int cx = iconCenterX - scale * 18, cy = iconCenterY;
    tft.drawRoundRect(cx - cw / 2, cy - ch / 2, cw, ch, scale * 4, bruceConfig.priColor);
    tft.fillCircle(cx, cy, scale * 4, bruceConfig.priColor);
    for (int k = 0; k < 3; k++) {
        int r = scale * (14 + k * 9);
        for (int w = 0; w < 2; w++)
            tft.drawArc(cx, cy, r + w, r + w - 1, 230, 310, cyberHue(300 + k * 60), bruceConfig.bgColor, false);
    }
}
