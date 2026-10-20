#include "SoundMenu.h"

#include "../cyd_pins.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/utils.h"
#include <Preferences.h>
#include <globals.h>
#include <math.h>

namespace {

// Duty (of 1024) per volume step: a narrower pulse is quieter. 0 = mute.
const uint16_t DUTY[] = {0, 6, 30, 120, 512};
constexpr int VOL_MAX = sizeof(DUTY) / sizeof(DUTY[0]) - 1;
constexpr uint8_t BITS = 10;

int volume = -1; // loaded lazily from NVS

int getVolume() {
    if (volume < 0) {
        Preferences p;
        p.begin("cybersnd", true);
        volume = constrain(p.getInt("vol", 2), 0, VOL_MAX);
        p.end();
    }
    return volume;
}

void setVolume(int v) {
    volume = constrain(v, 0, VOL_MAX);
    Preferences p;
    p.begin("cybersnd", false);
    p.putInt("vol", volume);
    p.end();
}

// The speaker gets its own LEDC timer: we retune it per note, so it must not
// share one with the backlight (5 kHz/8 bit) or the LED app. 1001 Hz/10 bit is
// an odd pair nothing else asks for, so ledcAttach can't match an existing timer.
bool spkOn = false;

bool spkBegin() {
    if (spkOn) return true;
    spkOn = ledcAttach(CYD_SPEAKER, 1001, BITS);
    if (spkOn) ledcWrite(CYD_SPEAKER, 0);
    return spkOn;
}

void spkTone(uint32_t hz) {
    if (!spkOn) return;
    if (!hz || !DUTY[getVolume()]) {
        ledcWrite(CYD_SPEAKER, 0);
        return;
    }
    ledcChangeFrequency(CYD_SPEAKER, hz, BITS);
    ledcWrite(CYD_SPEAKER, DUTY[getVolume()]);
}

void spkEnd() {
    if (!spkOn) return;
    ledcWrite(CYD_SPEAKER, 0);
    ledcDetach(CYD_SPEAKER);
    pinMode(CYD_SPEAKER, OUTPUT); // held low: a floating amp input hums
    digitalWrite(CYD_SPEAKER, LOW);
    spkOn = false;
}

// Wait `ms`, but give up early on ESC when `abortable`. False = aborted.
bool hold(uint32_t ms, bool abortable) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        if (abortable && check(EscPress)) return false;
        delay(2);
    }
    return true;
}

struct Note {
    uint16_t hz, ms; // hz 0 = rest
};

// The unicorn jingle: a rising C-major sparkle, then a shimmering trill.
const Note JINGLE[] = {
    {1047, 110}, {1319, 110}, {1568, 110}, {2093, 220}, {0, 50},   {1760, 90},
    {2093, 90},  {2637, 90},  {3136, 260}, {0, 40},     {2637, 55}, {3136, 55},
    {2637, 55},  {3136, 55},  {2637, 55},  {3136, 55},  {4186, 420},
};
const Note ALERT_OK[] = {
    {1319, 80}, {0, 20}, {1976, 180}
};
const Note ALERT_WARN[] = {
    {2000, 140}, {0, 70}, {2000, 140}, {0, 70}, {2000, 140}
};
const Note ALERT_ERR[] = {
    {880, 160}, {0, 30}, {440, 380}
};

template <size_t N> constexpr int len(const Note (&)[N]) { return N; }

// Log frequency (100 Hz-8 kHz) -> 0..1
float pitch(float hz) { return constrain(log2f(hz / 100.0f) / log2f(80.0f), 0.0f, 1.0f); }

constexpr int ROLL_X = 16, ROLL_Y = 52, ROLL_W = 288, ROLL_H = 130;

void drawHints(const char *a, const char *b) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(a, ROLL_X, ROLL_Y + ROLL_H + 8, 1);
    tft.drawString(b, ROLL_X + 150, ROLL_Y + ROLL_H + 8, 1);
}

void drawVolume() {
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawString("vol", ROLL_X + 226, ROLL_Y + ROLL_H + 8, 1);
    for (int i = 0; i < VOL_MAX; i++)
        tft.fillRect(
            ROLL_X + 248 + i * 10, ROLL_Y + ROLL_H + 4, 7, 12,
            i < getVolume() ? cyberHue(300 + i * 40) : 0x2104
        );
}

// Play a note list as a rainbow piano roll: each note a bar, height = pitch.
// title nullptr = sound only (the serial CLI must not draw over the screen).
bool playRoll(const char *title, const Note *notes, int n, bool abortable = true) {
    if (title) {
        drawMainBorderWithTitle(title);
        cyberRainbowFrame(ROLL_X - 2, ROLL_Y - 2, ROLL_W + 4, ROLL_H + 4, 0);
        drawHints(abortable ? "ESC stop" : "", "");
        drawVolume();
    }
    uint32_t total = 0;
    for (int i = 0; i < n; i++) total += notes[i].ms;
    if (!spkBegin()) {
        if (title) displayError("Speaker pin " + String(CYD_SPEAKER) + " busy", true);
        return false;
    }
    bool done = true;
    uint32_t at = 0;
    for (int i = 0; i < n && done; i++) {
        int x = ROLL_X + at * ROLL_W / total, w = max(2, (int)(notes[i].ms * ROLL_W / total) - 2);
        if (title && notes[i].hz) {
            float p = pitch(notes[i].hz);
            int h = 8 + p * (ROLL_H - 12);
            tft.fillRoundRect(x, ROLL_Y + ROLL_H - 2 - h, w, h, 2, cyberHue(270 + p * 330));
            // a sparkle on top of each note
            tft.fillCircle(x + w / 2, ROLL_Y + ROLL_H - 6 - h, 2, TFT_WHITE);
        }
        spkTone(notes[i].hz);
        done = hold(notes[i].ms, abortable);
        at += notes[i].ms;
    }
    spkTone(0);
    spkEnd();
    return done;
}

void sweep(bool draw) {
    if (draw) {
        drawMainBorderWithTitle("SWEEP");
        cyberRainbowFrame(ROLL_X - 2, ROLL_Y - 2, ROLL_W + 4, ROLL_H + 4, 0);
        drawHints("ESC stop", "");
        drawVolume();
    }
    if (!spkBegin()) return;
    const uint32_t ms = 3000;
    uint32_t t0 = millis();
    int lastX = -1;
    while (millis() - t0 < ms) {
        float k = (millis() - t0) / (float)ms;
        float hz = 100.0f * powf(80.0f, k); // 100 Hz -> 8 kHz, log
        spkTone((uint32_t)hz);
        if (!draw) {
            delay(8);
            continue;
        }
        int x = ROLL_X + k * ROLL_W;
        for (int i = lastX + 1; i <= x; i++) {
            float p = pitch(100.0f * powf(80.0f, (i - ROLL_X) / (float)ROLL_W));
            int h = 4 + p * (ROLL_H - 8);
            tft.drawFastVLine(i, ROLL_Y + ROLL_H - 2 - h, h, cyberHue(270 + p * 330));
        }
        lastX = x;
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.drawString(String((int)hz) + " Hz   ", ROLL_X + 6, ROLL_Y + 6, 1);
        if (check(EscPress)) break;
        delay(8);
    }
    spkEnd();
}

void testTone() {
    const uint16_t FREQS[] = {110, 220, 440, 880, 1000, 2000, 4000, 8000};
    constexpr int NF = sizeof(FREQS) / sizeof(FREQS[0]);
    int fi = 2;
    bool on = false;
    drawMainBorderWithTitle("TEST TONE");
    cyberRainbowFrame(ROLL_X - 2, ROLL_Y + 38, ROLL_W + 4, ROLL_H - 36, 0);
    drawHints("NEXT/PREV pitch", "SEL play/stop");
    drawVolume();
    if (!spkBegin()) return;
    bool redraw = true;
    int phase = 0;
    uint32_t lastAnim = 0;
    while (true) {
        if (redraw) {
            redraw = false;
            spkTone(on ? FREQS[fi] : 0);
            tft.setTextSize(3);
            tft.fillRect(ROLL_X, ROLL_Y, ROLL_W, 30, bruceConfig.bgColor);
            cyberRainbowText(String(FREQS[fi]) + " Hz", ROLL_X + 4, ROLL_Y + 4, fi * 40, bruceConfig.bgColor);
            tft.setTextSize(2);
            tft.setTextColor(on ? cyberHue(120) : TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString(on ? "PLAYING" : "stopped", ROLL_X + 200, ROLL_Y + 8, 1);
        }
        // Scrolling square wave; its period tracks the pitch.
        if (millis() - lastAnim > 60) {
            lastAnim = millis();
            phase += 4;
            int top = ROLL_Y + 50, h = ROLL_H - 60, y0 = top + h - 8, y1 = top + 8;
            int period = max(8, 96 >> fi);
            tft.fillRect(ROLL_X, top, ROLL_W, h, bruceConfig.bgColor);
            if (on) {
                int prev = -1;
                for (int x = 0; x < ROLL_W; x++) {
                    int y = ((x + phase) % period) < period / 2 ? y1 : y0;
                    uint16_t c = cyberHue(x + phase);
                    if (prev >= 0 && prev != y) tft.drawFastVLine(ROLL_X + x, min(y0, y1), h - 16, c);
                    else tft.drawPixel(ROLL_X + x, y, c);
                    prev = y;
                }
            } else {
                tft.drawFastHLine(ROLL_X, top + h / 2, ROLL_W, 0x4208);
            }
        }
        if (check(NextPress)) fi = min(fi + 1, NF - 1), redraw = true;
        if (check(PrevPress)) fi = max(fi - 1, 0), redraw = true;
        if (check(SelPress)) on = !on, redraw = true;
        if (check(EscPress)) break;
        delay(10);
    }
    spkEnd();
}

void alertsMenu() {
    options = {
        {"Success", []() { playRoll("SUCCESS", ALERT_OK, len(ALERT_OK)); }    },
        {"Warning", []() { playRoll("WARNING", ALERT_WARN, len(ALERT_WARN)); }},
        {"Error",   []() { playRoll("ERROR", ALERT_ERR, len(ALERT_ERR)); }    },
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Alerts");
}

} // namespace

void SoundMenu::optionsMenu() {
    int idx = 0;
    while (true) {
        options = {
            {"Test tone",      testTone                                                  },
            {"Unicorn jingle", []() { playRoll("UNICORN JINGLE", JINGLE, len(JINGLE)); }},
            {"Frequency sweep", []() { sweep(true); }                                    },
            {"Alerts",         alertsMenu                                                },
            {("Volume: " + String(getVolume()) + "/" + String(VOL_MAX)).c_str(),
             []() {
                 setVolume((getVolume() + 1) % (VOL_MAX + 1));
                 playRoll("VOLUME", ALERT_OK, len(ALERT_OK), false);
             }                                                                           },
        };
        addOptionToMainMenu();
        idx = loopOptions(options, MENU_TYPE_SUBMENU, "Sound", idx);
        if (idx < 0 || returnToMenu) break;
    }
}

void SoundMenu::drawIcon(float scale) {
    clearIconArea();
    // A speaker cone with rainbow sound waves.
    int cx = iconCenterX - scale * 14, cy = iconCenterY;
    int bw = scale * 8, bh = scale * 14;
    tft.fillRect(cx - bw, cy - bh / 2, bw, bh, bruceConfig.priColor);
    tft.fillTriangle(cx, cy - bh / 2, cx + scale * 14, cy - scale * 18, cx, cy + bh / 2, bruceConfig.priColor);
    tft.fillTriangle(cx, cy + bh / 2, cx + scale * 14, cy - scale * 18, cx + scale * 14, cy + scale * 18, bruceConfig.priColor);
    for (int i = 0; i < 3; i++) {
        int r = scale * (24 + i * 9);
        tft.drawArc(cx, cy, r, r - max(2, (int)(scale * 3)), 230, 310, cyberHue(300 + i * 60), bruceConfig.bgColor);
    }
}

static uint32_t toneCallback(cmd *c) {
    Command cmd(c);
    String a = cmd.getArgument("hz").getValue();
    int vol = cmd.getArgument("vol").getValue().toInt();
    int oldVol = getVolume();
    if (vol >= 0) volume = constrain(vol, 0, VOL_MAX); // this call only, not saved
    bool ok = true;
    if (a == "jingle") ok = playRoll(nullptr, JINGLE, len(JINGLE), false);
    else if (a == "sweep") sweep(false);
    else if (a == "ok") ok = playRoll(nullptr, ALERT_OK, len(ALERT_OK), false);
    else if (a == "alert") ok = playRoll(nullptr, ALERT_WARN, len(ALERT_WARN), false);
    else if (a == "error") ok = playRoll(nullptr, ALERT_ERR, len(ALERT_ERR), false);
    else {
        int hz = a.toInt();
        int ms = constrain((int)cmd.getArgument("ms").getValue().toInt(), 1, 10000);
        if (hz < 80 || hz > 20000 || !spkBegin()) ok = false;
        else {
            spkTone(hz);
            hold(ms, false);
            spkEnd();
        }
    }
    volume = oldVol;
    if (ok) {
        char buf[80];
        snprintf(
            buf, sizeof(buf), "tone %s on GPIO%d, volume %d/%d", a.c_str(), CYD_SPEAKER,
            vol >= 0 ? constrain(vol, 0, VOL_MAX) : oldVol, VOL_MAX
        );
        serialDevice->println(String(buf));
    } else serialDevice->println("usage: tone <80-20000 Hz> [ms] [vol 0-4] | jingle | sweep | ok | alert | error");
    return ok;
}

void createSoundCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("tone", toneCallback);
    cmd.addPosArg("hz", "440");
    cmd.addPosArg("ms", "300");
    cmd.addPosArg("vol", "-1");
}
