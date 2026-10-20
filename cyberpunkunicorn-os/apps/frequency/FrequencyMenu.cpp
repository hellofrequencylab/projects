#include "FrequencyMenu.h"

#include "../ui/cyber_icon.h"
#include "../ui/rainbow.h"
#include "../ui/scene.h"
#include "backend.h"
#include "card.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "fx.h"
#include "store.h"
#include <WiFi.h>
#include <globals.h>

using namespace freq;

// The design system's Frequency treatment (design/ui_kits/device/apps/Frequency.jsx):
// every screen on the synthwave scene, content on glass, sparkles always, confetti
// on success. The app opens on one screen that does the common thing: tap anything
// and it works out what it is. Member card -> check in (reader mode), venue or event
// tag -> where it points, blank tag -> offer to program it. SEL opens the menu.
namespace {

const char *ESC_KEY = "\x1B";
const char *ICON = "Frequency"; // menu name of the pixel-art FR logo

enum Key { K_NONE, K_SEL, K_ESC, K_PREV, K_NEXT };

Key readKey() {
    if (check(EscPress)) return K_ESC;
    if (check(SelPress)) return K_SEL;
    if (check(PrevPress)) return K_PREV;
    if (check(NextPress)) return K_NEXT;
    return K_NONE;
}

// Waits for a key with the scene animating. K_NONE after `timeoutMs` (0 = never).
Key waitKey(uint32_t timeoutMs = 0) {
    const uint32_t start = millis();
    delay(150);
    readKey(); // drop the press that opened this screen
    for (;;) {
        Key k = readKey();
        if (k != K_NONE) return k;
        if (returnToMenu) return K_ESC; // remote or serial asked for the main menu
        if (timeoutMs && millis() - start > timeoutMs) return K_NONE;
        cyberAnimate();
        delay(10);
    }
}

// A pick list (the skinned option list). The chosen index, or -1 on ESC.
int choose(const String &title, const std::vector<String> &labels) {
    int picked = -1;
    std::vector<Option> opts;
    for (size_t i = 0; i < labels.size(); i++)
        opts.push_back({labels[i].c_str(), [&picked, i]() { picked = i; }});
    loopOptions(opts, MENU_TYPE_SUBMENU, title.c_str());
    return picked;
}

// Text on the glass, centred.
void glassText(const String &text, int y, uint16_t color) {
    tft.setTextSize(1);
    tft.setTextColor(color, CY_GLASS);
    String t = text;
    int room = (tftWidth - 40) / LW;
    if ((int)t.length() > room) t = t.substring(0, room - 2) + "..";
    tft.drawCentreString(t, tftWidth / 2, y, 1);
}

// Fresh screen: border, status bar, scene, rainbow title on the top row.
void screen(const String &title, int horizon, int sunX = 160, int sunR = 0) {
    drawMainBorder(true);
    cyberScene(horizon, sunX, sunR);
    cyberHeadline(title, 30);
}

// One instruction, plain words, on the scene. For errors and "do this first".
Key notice(const String &title, const String &headline, const String &line1, const String &line2 = "",
           const String &footer = "any key: back", uint32_t timeoutMs = 0) {
    screen(title, 196, 160, 40);
    fxCentreText(headline, 66, 2, 0, cyberSkyAt(66));
    cyberPanel(14, 86, tftWidth - 28, line2.length() ? 40 : 28);
    glassText(line1, 96, TFT_WHITE);
    if (line2.length()) glassText(line2, 110, cyberHue(52));
    cyberIcon(ICON, tftWidth / 2, 160, 1, true);
    cyberKeepOut(tftWidth / 2 - 20, 144, 40, 32);
    cyberFooter(footer);
    cyberSparkles(22, 5);
    return waitKey(timeoutMs);
}

uint16_t kindColor(UrlKind k) {
    switch (k) {
        case URL_VENUE: return cyberHue(160);
        case URL_EVENT: return cyberHue(45);
        case URL_MEMBER: return cyberHue(300);
        default: return TFT_LIGHTGREY;
    }
}

// ---- the reader ------------------------------------------------------------------

// Claims the PN532, or shows the DIP guide. False = the user backed out.
bool openReader(bool &anyway) {
    anyway = false;
    for (;;) {
        CardStatus s = readerOpen();
        if (s == CARD_OK) return true;
        if (s != CARD_NO_READER) {
            notice("FREQUENCY", "READER BUSY", cardStatusText(s), "Close the other screen or serial command.");
            return false;
        }
        int c = cyberHardwareCard("FREQUENCY", "TURN DIP 3 ON", "PN532 (NFC)", "I2C 27/22", 0b000100, 0b011011);
        if (c == 1) anyway = true;
        if (c != 0) return false;
    }
}

bool openReader() {
    bool anyway;
    return openReader(anyway);
}

// Holds until tag `uid` has left the field, so a loop doesn't re-read (or
// re-write) the tag still lying on the reader. Animates meanwhile.
void waitRemoved(const String &uid) {
    CardInfo c;
    uint32_t start = millis();
    for (int misses = 0; misses < 2 && millis() - start < 20000;) {
        if (pollCard(c) == CARD_OK && c.uid == uid) misses = 0;
        else misses++;
        if (check(EscPress)) return;
        cyberAnimate();
        delay(40);
    }
}

// ---- what a tag holds -----------------------------------------------------------

String badgeFor(const CardInfo &c, const UrlCheck &u) {
    if (!c.ntag) return "NOT AN NTAG";
    if (c.uri.isEmpty()) return "BLANK TAG";
    switch (u.kind) {
        case URL_VENUE: return "VENUE TAG";
        case URL_EVENT: return "EVENT TAG";
        case URL_MEMBER: return "MEMBER CARD";
        default: return "OTHER LINK";
    }
}

// Tag details on glass, the logo on the right. Used after a tap and a write.
void showTag(const CardInfo &c, const String &title, const String &headline, bool burst) {
    UrlCheck u = checkUrl(c.uri);
    screen(title, 192, 276, 26);
    fxCentreText(headline, 54, 2, 0, cyberSkyAt(54));
    cyberPanel(8, 66, 230, 120);
    int y = 72;
    cyberLine(14, y, "Tag", c.type + (c.locked ? "  LOCKED" : ""), c.locked ? cyberHue(330) : TFT_WHITE);
    cyberLine(14, y += 12, "UID", c.uid, TFT_LIGHTGREY);
    if (c.uri.length()) {
        cyberLine(14, y += 12, "Points to", u.host, cyberHue(180));
        cyberLine(14, y += 12, "Path", u.path, kindColor(u.kind));
        if (u.kind == URL_VENUE) cyberLine(14, y += 12, "Signed", "yes (?s=v1...)", cyberHue(120));
        if (u.kind == URL_EVENT || u.kind == URL_VENUE)
            cyberLine(14, y += 12, "NFC mark", u.fixed == c.uri ? "?m=nfc ok" : "missing ?m=nfc",
                      u.fixed == c.uri ? cyberHue(120) : cyberHue(30));
        if (!u.ok) cyberLine(14, y += 12, "Problem", u.problem, cyberHue(30));
    } else if (c.ntag) {
        cyberLine(14, y += 12, "Holds", "nothing yet", TFT_LIGHTGREY);
    } else {
        cyberLine(14, y += 12, "Use", "NTAG213/215/216 stickers", cyberHue(52));
    }
    if (c.ntag) cyberLine(14, y += 12, "Room", String(c.capacity) + " bytes NDEF", cyberHue(52));
    const char *what = !c.ntag              ? "This reader writes NTAGs only."
                       : c.uri.isEmpty()    ? "Ready to become a venue tag."
                       : u.kind == URL_VENUE ? "Opens the Space check-in."
                       : u.kind == URL_EVENT ? "Opens the event door code."
                       : u.kind == URL_MEMBER ? "A member's Frequency card."
                                              : "Not a Frequency link.";
    tft.setTextColor(TFT_WHITE, CY_GLASS);
    tft.drawString(what, 14, 172, 1);
    cyberIcon(ICON, 276, 110, 2, true);
    cyberKeepOut(240, 84, 72, 54);
    cyberSparkles(20, 11);
    if (burst) cyberBurst(276, 110);
}

// ---- writing ---------------------------------------------------------------------

// Pick list from tags.txt, or a typed URL. False on ESC.
bool pickUrl(String &label, String &url) {
    String from;
    std::vector<TagSource> list = loadTagList(from);
    std::vector<String> labels;
    for (const TagSource &t : list) labels.push_back(t.label);
    labels.push_back("Type a URL...");
    labels.push_back("How to add links");
    int i = choose(list.empty() ? String("PICK A LINK") : "LINKS FROM " + from, labels);
    if (i < 0) return false;
    if (i < (int)list.size()) {
        label = list[i].label;
        url = list[i].url;
        return true;
    }
    if (i == (int)list.size() + 1) {
        ScrollableTextArea area("ADD LINKS");
        area.addLine("Venue link: on frequencylocal.com,");
        area.addLine("Space settings > Check-in > copy link.");
        area.addLine("");
        area.addLine("Keep a list: one per line,");
        area.addLine("  label|url");
        area.addLine("in " + String(TAG_LIST_PATH) + " on SD or");
        area.addLine("LittleFS (web UI file manager).");
        area.addLine("");
        area.addLine("Or over serial (115200):");
        area.addLine("  frequency tag <url>");
        area.show();
        return false;
    }
    url = keyboard("https://frequencylocal.com/", 200, "Check-in or event URL");
    if (url == ESC_KEY || url.isEmpty()) return false;
    label = "";
    return true;
}

// Checks a URL before writing. False = the user cancelled.
bool vetUrl(const String &url, String &label, UrlCheck &u) {
    u = checkUrl(url);
    if (!u.ok || u.kind == URL_MEMBER) {
        String why = u.kind == URL_MEMBER ? "That is a member card link" : u.problem;
        if (choose("ODD LINK: " + why, {"Cancel", "Write it anyway"}) != 1) return false;
    }
    if (label.isEmpty()) {
        label = keyboard("", 24, "Label (table, door...)");
        if (label == ESC_KEY) return false;
        if (label.isEmpty()) label = kindName(u.kind);
    }
    return true;
}

String writeProblem(CardStatus s) {
    switch (s) {
        case CARD_TOO_LONG: return "Link too long for this tag: use an NTAG215.";
        case CARD_LOCKED: return "This tag is locked forever. Use a new one.";
        case CARD_NOT_NTAG: return "Not an NTAG. Use NTAG213/215/216.";
        case CARD_VERIFY_FAILED: return "Wrote, but it reads back wrong. Tap again.";
        default: return String(cardStatusText(s)) + ". Hold it still and tap again.";
    }
}

// The batch writer: one screen, tap tag after tag, no dialogs in between.
// The reader must already be open. Returns how many were written.
int writeLoop(const String &label, const UrlCheck &u) {
    int written = 0;
    String status = "TAP A TAG";
    uint16_t statusColor = 0;
    for (;;) {
        screen("PROGRAM TAGS", 176);
        fxCentreText(status, 56, 2, 0, cyberSkyAt(56));
        if (statusColor) { // a problem: plain pink headline instead of rainbow
            tft.setTextSize(2);
            int w = status.length() * LW * 2;
            tft.setTextColor(statusColor, cyberSkyAt(56));
            tft.drawString(status, (tftWidth - w) / 2, 48, 1);
            tft.setTextSize(1);
        }
        cyberPanel(14, 68, tftWidth - 28, 18);
        glassText(label + "  " + u.host + u.path, 73, TFT_WHITE);
        cyberIcon(ICON, tftWidth / 2, 128, 1, true);
        cyberKeepOut(tftWidth / 2 - 22, 112, 44, 34);
        cyberFooter(written ? "ESC done   " + String(written) + " written" : "ESC back");
        cyberSparkles(18, 9, 88);
        fxWaveReset();

        CardInfo c;
        int t = 0;
        CardStatus s = waitCard(
            c, 0, []() { return returnToMenu || check(EscPress); },
            [&]() {
                fxWave(12, 92, tftWidth - 24, 78, t++);
                cyberAnimate();
            }
        );
        if (s != CARD_OK) return written;

        CardInfo done;
        s = writeCard(u.fixed, done);
        if (s == CARD_OK) {
            logWrite(label, checkUrl(done.uri), done.uid, false);
            written++;
            showTag(done, "TAG WRITTEN", "NAILED IT", true);
            cyberFooter("Lift it off for the next one");
            waitRemoved(done.uid);
            status = "TAP THE NEXT ONE";
            statusColor = 0;
        } else {
            status = s == CARD_TOO_LONG ? "TOO LONG" : s == CARD_LOCKED ? "TAG IS LOCKED" : "TRY AGAIN";
            statusColor = cyberHue(330);
            notice("PROGRAM TAGS", status, writeProblem(s), "", "any key: try again", 4000);
            waitRemoved(c.uid);
        }
    }
}

void programTags() {
    String label, url;
    UrlCheck u;
    if (!pickUrl(label, url) || !vetUrl(url, label, u)) return;
    if (!openReader()) return;
    int n = writeLoop(label, u);
    readerClose();
    if (n) notice("PROGRAM TAGS", "ALL DONE", String(n) + (n == 1 ? " tag" : " tags") + " written.",
                  "Tap one with a phone to try it.", "any key: back", 6000);
}

// Programs the blank tag that was just tapped on the home screen.
void programThisTag(const CardInfo &blank) {
    String label, url;
    UrlCheck u;
    if (!pickUrl(label, url) || !vetUrl(url, label, u)) return;
    CardInfo done;
    CardStatus s = writeCard(u.fixed, done);
    if (s != CARD_OK) {
        notice("PROGRAM TAG", "DIDN'T WRITE", writeProblem(s));
        return;
    }
    logWrite(label, checkUrl(done.uri), done.uid, false);
    showTag(done, "TAG WRITTEN", "NAILED IT", true);
    cyberFooter("any key: done");
    waitKey(8000);
    cyberBurst(-1, -1);
}

// ---- lock ------------------------------------------------------------------------

void lockATag() {
    if (choose("LOCK IS FOREVER", {"Back", "I understand, continue"}) != 1) return;
    if (!openReader()) return;
    screen("LOCK A TAG", 176);
    fxCentreText("TAP THE TAG", 56, 2, 0, cyberSkyAt(56));
    cyberPanel(14, 68, tftWidth - 28, 18);
    glassText("It becomes read-only. It can never change.", 73, cyberHue(52));
    cyberIcon(ICON, tftWidth / 2, 128, 1, true);
    cyberKeepOut(tftWidth / 2 - 22, 112, 44, 34);
    cyberFooter("ESC back");
    cyberSparkles(18, 4, 88);
    fxWaveReset();
    CardInfo c;
    int t = 0;
    CardStatus s = waitCard(
        c, 0, []() { return returnToMenu || check(EscPress); },
        [&]() {
            fxWave(12, 92, tftWidth - 24, 78, t++);
            cyberAnimate();
        }
    );
    if (s == CARD_OK) {
        if (c.locked) {
            notice("LOCK A TAG", "ALREADY LOCKED", "This tag was locked before.");
        } else if (choose("LOCK " + c.uid + "?", {"No, keep it writable", "Yes, lock it forever"}) == 1) {
            CardStatus ls = lockCard(c.uid);
            if (ls == CARD_OK) {
                logWrite("locked", checkUrl(c.uri), c.uid, true);
                notice("LOCK A TAG", "LOCKED", "Read-only for good.", "", "any key: back", 6000);
            } else {
                notice("LOCK A TAG", "NOT LOCKED", String(cardStatusText(ls)) + ". Hold it still.");
            }
        }
    }
    readerClose();
}

// ---- members ---------------------------------------------------------------------

void celebrateScan(const ScanReply &r) {
    screen("CHECKED IN", 196, 268, 30);
    fxCentreText(r.name, 60, 3, 0, cyberSkyAt(60));
    int y = 80;
    if (r.zaps) {
        tft.setTextSize(2);
        tft.setTextColor(cyberHue(55), cyberSkyAt(y));
        tft.drawCentreString("+" + String(r.zaps) + " ZAPS", tftWidth / 2, y, 1);
        tft.setTextSize(1);
        cyberKeepOut(tftWidth / 2 - 60, y - 4, 120, 24);
        y += 22;
    }
    if (r.hasLoyalty && r.required > 0) {
        int stamps = constrain(r.stamps, 0, r.required);
        if (r.earned && stamps == 0) stamps = r.required; // card rolled over: show it full
        const int rows = r.required > 6 ? 2 : 1, cell = min((tftWidth - 40) / (r.required > 6 ? (r.required + 1) / 2 : r.required), 44);
        const int ph = rows * cell + 22;
        cyberPanel(10, y, tftWidth - 20, ph);
        uint16_t bg = bruceConfig.bgColor; // the stamp card paints on the glass
        bruceConfig.bgColor = CY_GLASS;
        fxStampCard(14, y + 4, tftWidth - 28, r.required, max(0, stamps - 1), stamps);
        bruceConfig.bgColor = bg;
        glassText(String(stamps) + " / " + String(r.required) + " stamps", y + ph - 12, TFT_WHITE);
    }
    cyberSparkles(18, 2, 58);
    cyberBurst(268, 150);
    if (r.earned) {
        cyberFooter("Reward next...");
        waitKey(1500);
        cyberBurst(-1, -1);
        fxBurst(tftWidth / 2, tftHeight / 2);
        screen("REWARD!", 196, 160, 44);
        for (int f = 0; f < 10; f++) {
            fxUnicorn(tftWidth / 2 - UNICORN_W * 3 / 2, 50, 3, f & 1, f * 36);
            delay(80);
        }
        cyberKeepOut(tftWidth / 2 - UNICORN_W * 3 / 2, 46, UNICORN_W * 3, UNICORN_H * 3 + 8);
        cyberPanel(30, 142, tftWidth - 60, 44);
        tft.setTextSize(2);
        cyberRainbowText("REWARD EARNED", (tftWidth - 13 * LW * 2) / 2, 148, 0, CY_GLASS);
        tft.setTextSize(1);
        glassText(r.rewardCode.length() ? "Code: " + r.rewardCode : "Show this screen to staff.", 170, cyberHue(52));
        cyberSparkles(26, 8, 46);
        cyberBurst(tftWidth / 2, 100);
    }
    cyberFooter("any key: next member");
    waitKey(r.earned ? 20000 : 6000);
    cyberBurst(-1, -1);
}

// A member card was tapped on the home screen.
void checkIn(const CardInfo &c, const UrlCheck &u) {
    screen("CHECKING IN", 196, 160, 40);
    fxCentreText("ONE SEC...", 70, 2, 0, cyberSkyAt(70));
    ScanReply r = postScan(u.token, c.uid);
    switch (r.status) {
        case API_OK: {
            celebrateScan(r);
            String err;
            if (queuedScans().size()) retryQueue(err);
            break;
        }
        case API_OFFLINE:
            if (queueScan({nowEpoch(), c.uid, u.token}))
                notice("CHECK-IN", "SAVED FOR LATER", "No connection right now.", "It sends itself when Wi-Fi is back.",
                       "any key: next member", 5000);
            else
                notice("CHECK-IN", "QUEUE FULL", String(QUEUE_MAX) + " check-ins are waiting.",
                       "Join Wi-Fi to send them, then tap again.");
            break;
        default: {
            Words w = reasonWords(r.reason);
            String who = r.name.length() && r.reason == "already_captured" ? "Hi " + r.name + "!" : "";
            notice("CHECK-IN", w.title, w.hint, who, "any key: next member", 6000);
            break;
        }
    }
}

// ---- home ------------------------------------------------------------------------

void drawHome() {
    Settings &s = settings();
    const bool ready = s.readerReady();
    const size_t queued = queuedScans().size();
    screen("FREQUENCY", 140, 160, 34);
    cyberIcon(ICON, tftWidth / 2, 82, 3, true);
    cyberKeepOut(tftWidth / 2 - 52, 42, 104, 80);
    cyberPanel(30, 146, tftWidth - 60, 50);
    tft.setTextSize(2);
    cyberRainbowText("TAP A TAG", (tftWidth - 9 * LW * 2) / 2, 153, cyberPhase(), CY_GLASS);
    tft.setTextSize(1);
    String hint = !ready                          ? "It shows you where any tag points."
                  : WiFi.status() == WL_CONNECTED ? "Members tap their card to check in."
                                                  : "Offline: check-ins are saved for later.";
    glassText(hint, 178, ready ? cyberHue(160) : TFT_LIGHTGREY);
    cyberFooter(queued ? "SEL menu   ESC exit   " + String(queued) + " waiting" : "SEL menu   ESC exit");
    cyberSparkles(26, 7);
}

void showWrites();
void showQueue();
void editSettings();
void issueMemberCard();

// The menu behind SEL. True = go back to the tap screen, false = leave the app.
bool menu() {
    for (;;) {
        size_t queued = queuedScans().size();
        std::vector<String> items = {"Program tags", "Issue member card", "Lock a tag", "Tags written"};
        if (queued) items.push_back("Waiting check-ins: " + String(queued));
        items.push_back("Settings");
        int i = choose("Frequency", items);
        if (i < 0) return true;
        const String &pick = items[i];
        if (pick == "Program tags") programTags();
        else if (pick == "Issue member card") issueMemberCard();
        else if (pick == "Lock a tag") lockATag();
        else if (pick == "Tags written") showWrites();
        else if (pick == "Settings") editSettings();
        else showQueue();
    }
}

void home() {
    for (;;) {
        bool anyway;
        if (!openReader(anyway)) {
            if (anyway && menu()) continue; // "Open anyway": the menu works without the reader
            return;
        }
        drawHome();
        CardInfo c;
        Key k = K_NONE;
        CardStatus s = waitCard(
            c, 0,
            [&]() {
                k = returnToMenu ? K_ESC : readKey();
                return k == K_SEL || k == K_ESC;
            },
            []() { cyberAnimate(); }
        );
        if (s != CARD_OK) {
            readerClose();
            if (k == K_ESC || returnToMenu) return;
            if (!menu()) return;
            continue;
        }

        UrlCheck u = checkUrl(c.uri);
        if (u.kind == URL_MEMBER && u.ok && settings().readerReady()) {
            checkIn(c, u);
        } else {
            const bool blank = c.ntag && c.uri.isEmpty();
            showTag(c, "TAG CHECK", badgeFor(c, u), false);
            String foot = blank ? "SEL make it a venue tag   ESC back" : "any key: back";
            if (u.kind == URL_MEMBER && !settings().readerReady()) foot = "Reader mode is off: Settings";
            cyberFooter(foot);
            Key k2 = waitKey(15000);
            if (blank && k2 == K_SEL) programThisTag(c);
        }
        waitRemoved(c.uid);
        readerClose();
    }
}

// ---- member cards, logs, queue, settings -----------------------------------------------

void explainReaderMode() {
    ScrollableTextArea area("READER MODE");
    area.addLine("Reader mode lets members check in");
    area.addLine("by tapping their card on this unit.");
    area.addLine("");
    area.addLine("1. frequencylocal.com: your Space >");
    area.addLine("   Check-in & scanners. Add this");
    area.addLine("   device, copy its ID and secret.");
    area.addLine("2. Here: Settings > Device ID and");
    area.addLine("   Device secret, then Reader mode.");
    area.addLine("3. Join Wi-Fi: Config > WiFi.");
    area.addLine("");
    area.addLine("No Wi-Fi? Check-ins are saved and");
    area.addLine("sent when it comes back.");
    area.show();
}

void issueMemberCard() {
    if (!settings().readerReady()) {
        explainReaderMode();
        return;
    }
    String handle = keyboard("", 40, "Member handle");
    if (handle == ESC_KEY || handle.isEmpty()) return;
    screen("ISSUE CARD", 196, 160, 40);
    fxCentreText("ASKING...", 70, 2, 0, cyberSkyAt(70));
    IssueReply r = issueCard(handle);
    if (r.status != API_OK) {
        Words w = reasonWords(r.reason);
        notice("ISSUE CARD", w.title, w.hint);
        return;
    }
    if (!openReader()) return;
    UrlCheck u = checkUrl(r.url);
    for (;;) {
        screen("ISSUE CARD", 176);
        fxCentreText("TAP A BLANK CARD", 56, 2, 0, cyberSkyAt(56));
        cyberPanel(14, 68, tftWidth - 28, 18);
        glassText("for " + handle, 73, cyberHue(300));
        cyberIcon(ICON, tftWidth / 2, 128, 1, true);
        cyberKeepOut(tftWidth / 2 - 22, 112, 44, 34);
        cyberFooter("ESC back");
        cyberSparkles(18, 6, 88);
        fxWaveReset();
        CardInfo c, done;
        int t = 0;
        CardStatus s = waitCard(
            c, 0, []() { return returnToMenu || check(EscPress); },
            [&]() {
                fxWave(12, 92, tftWidth - 24, 78, t++);
                cyberAnimate();
            }
        );
        if (s != CARD_OK) break;
        s = writeCard(u.fixed.length() ? u.fixed : r.url, done);
        if (s != CARD_OK) {
            notice("ISSUE CARD", "TRY AGAIN", writeProblem(s), "", "any key: try again", 4000);
            waitRemoved(c.uid);
            continue;
        }
        showTag(done, "CARD READY", handle, true);
        cyberFooter("any key: done");
        waitKey(8000);
        cyberBurst(-1, -1);
        break;
    }
    readerClose();
}

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
    if (choose(String(q.size()) + " WAITING", {"Send them now", "Back"}) != 0) return;
    screen("CHECK-INS", 196, 160, 40);
    fxCentreText("SENDING...", 70, 2, 0, cyberSkyAt(70));
    String err;
    int n = retryQueue(err);
    if (err.length()) notice("CHECK-INS", String(n) + " SENT", "Stopped: " + reasonWords(err).hint, "The rest stay saved.");
    else notice("CHECK-INS", "ALL SENT", String(n) + (n == 1 ? " check-in" : " check-ins") + " delivered.", "",
                "any key: back", 5000);
}

void editSettings() {
    for (;;) {
        Settings &s = settings();
        int i = choose(
            "SETTINGS",
            {String("Reader mode: ") + (s.readerMode ? "ON" : "OFF"),
             "Server: " + s.baseUrl,
             "Device ID: " + (s.deviceId.length() ? s.deviceId : String("not set")),
             String("Device secret: ") + (s.deviceSecret.length() ? "set" : "not set"),
             "How reader mode works"}
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
            case 4: explainReaderMode(); continue;
            default: return;
        }
        s.save();
    }
}

} // namespace

void FrequencyMenu::optionsMenu() {
    storeBegin();
    home();
    returnToMenu = true;
}

void FrequencyMenu::drawIcon(float scale) {
    clearIconArea();
    cyberIcon(ICON, iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}
