#include "scene.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "cyber_icon.h"
#include "rainbow.h"
#include <globals.h>
#include <math.h>

namespace {

struct Rect {
    int16_t x, y, w, h;
};

struct Scene {
    bool on = false;
    int horizon = 0, sunX = 0, sunR = 0;
    Rect keep[8];
    int keeps = 0;
    // sparkles
    int count = 0, seed = 0, bottom = 0;
    uint32_t last = 0;
    int t = 0;
    // confetti
    int bx = -1, by = -1;
} sc;

const uint16_t STAR[5] = {0xFFFF, 0x07FF, 0xFAD9, 0xFEE7, 0x07F4}; // white, cyan, pink, sun yellow, neon green
const uint16_t MAGENTA = 0xF81F;

uint16_t rgb(int r, int g, int b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

int left() { return 6; }
int right() { return tftWidth - 7; }
int bottomRow() { return tftHeight - 7; }

bool blocked(int x, int y, int w, int h) {
    for (int i = 0; i < sc.keeps; i++) {
        const Rect &k = sc.keep[i];
        if (x < k.x + k.w && x + w > k.x && y < k.y + k.h && y + h > k.y) return true;
    }
    if (sc.sunR) { // the sun's bounding box
        int sx = sc.sunX - sc.sunR, sy = sc.horizon - sc.sunR;
        if (x < sx + 2 * sc.sunR && x + w > sx && y < sc.horizon && y + h > sy) return true;
    }
    return x < left() || x + w > right() || y < CY_TOP || y + h > sc.bottom;
}

// Horizontal span clipped to the content area, on the floor.
void span(int x0, int x1, int y, uint16_t c) {
    if (x0 > x1) std::swap(x0, x1);
    x0 = max(x0, left());
    x1 = min(x1, right());
    if (x1 >= x0 && y >= 27 && y <= bottomRow()) tft.drawFastHLine(x0, y, x1 - x0 + 1, c);
}

void star(int x, int y, int r, uint16_t c, bool erase) {
    if (erase) {
        tft.drawFastHLine(x - r, y, 2 * r + 1, cyberSkyAt(y));
        for (int yy = y - r; yy <= y + r; yy++) tft.drawPixel(x, yy, cyberSkyAt(yy));
        if (r > 2)
            for (int yy = y - 1; yy <= y + 1; yy++) tft.drawFastHLine(x - 1, yy, 3, cyberSkyAt(yy));
        return;
    }
    tft.drawFastHLine(x - r, y, 2 * r + 1, c);
    tft.drawFastVLine(x, y - r, 2 * r + 1, c);
    if (r > 2) tft.fillRect(x - 1, y - 1, 3, 3, TFT_WHITE);
}

int starRadius(int i, int t) {
    int ph = (t + i * 5) % 18;
    int r = ph < 3 ? ph : ph < 6 ? 6 - ph : 0;
    return r ? r + (i % 3 == 0 ? 1 : 0) : 0;
}

void starAt(int i, int &x, int &y) {
    x = left() + 6 + (i * 97 + sc.seed * 31) % (right() - left() - 12);
    y = CY_TOP + 4 + (i * 53 + sc.seed * 17) % max(1, sc.bottom - CY_TOP - 8);
}

void particle(int i, int t, int &x, int &y) {
    float a = i * 10 * DEG_TO_RAD;
    int d = ((t * 4 + i * 7) % 90) + 6;
    x = sc.bx + (int)(cosf(a) * d);
    y = sc.by + (int)(sinf(a) * d * 0.7f);
}

void sparkleStep() {
    const int t = sc.t;
    for (int i = 0; i < sc.count; i++) {
        int x, y;
        starAt(i, x, y);
        int r0 = starRadius(i, t - 1), r1 = starRadius(i, t);
        if (r0 == r1 && r0 == 0) continue;
        if (blocked(x - 4, y - 4, 9, 9)) continue;
        if (r0) star(x, y, r0, 0, true);
        if (r1) star(x, y, r1, STAR[i % 5], false);
    }
    if (sc.bx < 0) return;
    for (int i = 0; i < 36; i++) {
        int x, y;
        particle(i, t - 1, x, y);
        if (!blocked(x - 2, y - 2, 6, 6))
            for (int yy = y; yy < y + 3; yy++) tft.drawFastHLine(x, yy, 3, cyberSkyAt(yy));
        particle(i, t, x, y);
        if (!blocked(x - 2, y - 2, 6, 6)) tft.fillRect(x, y, 3, 3, cyberHue(i * 40 + t * 12));
    }
}

} // namespace

int cyFooterY() { return tftHeight - 16; }

bool cyberBlocked(int x, int y, int w, int h) { return blocked(x, y, w, h); }

void cyberSceneEnd() { sc = Scene(); }

uint16_t cyberSkyAt(int y) {
    if (!sc.on) return bruceConfig.bgColor;
    if (y >= sc.horizon) return TFT_BLACK;
    int k = max(0, y) * 255 / max(1, sc.horizon);
    return rgb(10 + k * 90 / 255, k * 10 / 255, 40 + k * 60 / 255);
}

void cyberScene(int horizon, int sunX, int sunR) {
    sc = Scene();
    sc.on = true;
    sc.horizon = horizon;
    sc.sunX = sunX;
    sc.sunR = sunR;
    sc.bottom = horizon;
    const int x0 = left(), w = right() - left() + 1;
    for (int y = 27; y < horizon; y++) tft.drawFastHLine(x0, y, w, cyberSkyAt(y));
    // Striped sun: yellow at the top to pink at the horizon, cut by widening bands near the base.
    for (int dy = -sunR; dy < 0 && sunR > 0; dy++) {
        int band = -dy;
        if (band < sunR * 47 / 100 && (band % 5) < (5 - band / max(1, sunR / 10))) continue;
        int half = (int)sqrtf(sunR * sunR - dy * dy);
        int k = (dy + sunR) * 255 / sunR;
        span(sunX - half, sunX + half - 1, horizon + dy, rgb(255, 220 - k * 180 / 255, 60 + k * 120 / 255));
    }
    tft.fillRect(x0, horizon, w, bottomRow() - horizon + 1, TFT_BLACK);
    for (int x = x0; x <= right(); x += 2) tft.drawFastHLine(x, horizon, 2, cyberHue(x * 360 / tftWidth));
    for (int i = 1, y = horizon + 3; y <= bottomRow(); i++, y += 2 + i * 2) span(x0, right(), y, cyberHue(300));
    // Perspective rays: one span per row so they stay unbroken and clipped.
    const int cx = tftWidth / 2, depth = tftHeight + 20 - horizon;
    for (int i = -8; i <= 8; i++) {
        int prev = cx + i * 12;
        for (int y = horizon + 1; y <= bottomRow(); y++) {
            int x = cx + i * 12 + (i * 48) * (y - horizon) / depth;
            span(prev, x, y, cyberHue(300));
            prev = x;
        }
    }
}

void cyberKeepOut(int x, int y, int w, int h) {
    if (sc.keeps < 8) sc.keep[sc.keeps++] = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h};
}

void cyberPanel(int x, int y, int w, int h) {
    tft.fillRect(x + 1, y + 1, w - 2, h - 2, CY_GLASS);
    cyberRainbowFrame(x, y, w, h, 300);
    cyberKeepOut(x - 4, y - 4, w + 8, h + 8);
}

void cyberHeadline(const String &text, int y, int size) {
    String t = text;
    t.toUpperCase();
    while (size > 1 && (int)t.length() * LW * size > tftWidth - 24) size--;
    tft.setTextSize(size);
    int w = t.length() * LW * size, x = (tftWidth - w) / 2;
    if (sc.on) // letters over the sky: paint their cells in the sky colour first
        for (int yy = y; yy < y + 8 * size; yy++) tft.drawFastHLine(x, yy, w, cyberSkyAt(yy));
    for (unsigned i = 0; i < t.length(); i++) { // transparent background: the sky shows through
        tft.setTextColor(cyberHue(cyberPhase() + i * 25));
        tft.drawString(String(t[i]), x + i * LW * size, y, 1);
    }
    tft.setTextSize(1);
    cyberKeepOut(x - 4, y - 4, w + 8, 8 * size + 8);
}

void cyberFooter(const String &text) {
    const int y = cyFooterY();
    tft.setTextSize(1);
    tft.fillRect(left(), y, right() - left(), 8, sc.on ? TFT_BLACK : bruceConfig.bgColor);
    tft.setTextColor(TFT_LIGHTGREY, sc.on ? TFT_BLACK : bruceConfig.bgColor);
    tft.drawCentreString(text, tftWidth / 2, y, 1);
    cyberKeepOut(left(), y - 4, right() - left(), 12);
}

void cyberLine(int x, int y, const String &label, const String &value, uint16_t color) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, CY_GLASS);
    tft.drawString(label, x, y, 1);
    String v = value;
    int room = (tftWidth - x - 68 - 16) / LW;
    if ((int)v.length() > room) v = v.substring(0, max(1, room - 2)) + "..";
    tft.setTextColor(color, CY_GLASS);
    tft.drawString(v, x + 68, y, 1);
}

void cyberSparkles(int count, int seed, int bottom) {
    sc.count = count;
    sc.seed = seed;
    sc.bottom = bottom > 0 ? bottom : sc.horizon - 4;
    sc.t = 0;
    sc.last = millis();
}

void cyberBurst(int cx, int cy) {
    if (cx < 0 && sc.bx >= 0) { // stop: wipe the last frame
        for (int i = 0; i < 36; i++) {
            int x, y;
            particle(i, sc.t, x, y);
            if (!blocked(x - 2, y - 2, 6, 6))
                for (int yy = y; yy < y + 3; yy++) tft.drawFastHLine(x, yy, 3, cyberSkyAt(yy));
        }
    }
    sc.bx = cx;
    sc.by = cy;
}

void cyberAnimate() {
    cyberIconIdle();
    if (!sc.on || (!sc.count && sc.bx < 0) || millis() - sc.last < 70) return;
    sc.last = millis();
    sc.t++;
    sparkleStep();
}

void cyberDipSwitch(int x, int y, uint8_t onMask, uint8_t offMask) {
    const int sw = 24, gap = 8;
    tft.setTextSize(1);
    for (int i = 0; i < 6; i++) {
        const int sx = x + i * (sw + gap);
        const bool on = onMask & (1 << i), off = offMask & (1 << i);
        const uint16_t c = on ? cyberHue(300) : off ? TFT_DARKGREY : 0x4208;
        tft.fillRect(sx, y, sw, 46, on ? 0x3006 : CY_GLASS);
        tft.drawRect(sx, y, sw, 46, c);
        if (on) {
            tft.drawRect(sx - 1, y - 1, sw + 2, 48, cyberHue(330)); // glow
            for (int k = 0; k < 20; k++) tft.drawFastHLine(sx + 2, y + 2 + k, sw - 4, cyberHue(300 + k * 3 / 2));
        } else if (off) {
            tft.fillRect(sx + 2, y + 24, sw - 4, 20, c);
        }
        tft.setTextColor(on ? TFT_WHITE : c, CY_GLASS);
        tft.drawCentreString(String(i + 1), sx + sw / 2, y + 51, 1);
    }
}

int cyberHardwareCard(
    const String &title, const String &instruction, const String &module, const String &pins, uint8_t onMask,
    uint8_t offMask
) {
    static const char *const LABELS[3] = {"Check again", "Open anyway", "Back"};
    drawMainBorder(true);
    cyberScene(198, 160, 44);
    cyberHeadline(title, 30, 2);
    cyberHeadline(instruction, 48, 2);
    cyberPanel(58, 64, 204, 72);
    cyberDipSwitch(68, 68, onMask, offMask);
    cyberPanel(14, 140, tftWidth - 28, 32);
    tft.setTextColor(TFT_WHITE, CY_GLASS);
    tft.drawCentreString(module + " on " + pins, tftWidth / 2, 145, 1);
    tft.setTextColor(cyberHue(52), CY_GLASS);
    tft.drawCentreString("Only one of DIP 1-5 may be ON.", tftWidth / 2, 158, 1);
    cyberKeepOut(14, 176, tftWidth - 28, 22);
    cyberSparkles(22, 3);

    int sel = 0;
    bool redraw = true;
    for (;;) {
        if (redraw) {
            const int bw = 92, gap = 6, x0 = (tftWidth - 3 * bw - 2 * gap) / 2, y = 180;
            for (int i = 0; i < 3; i++) {
                const int bx = x0 + i * (bw + gap);
                const uint16_t c = cyberItemColor(i, 3);
                tft.fillRect(bx, y, bw, 16, i == sel ? c : TFT_BLACK);
                tft.drawRect(bx, y, bw, 16, c);
                tft.setTextColor(i == sel ? TFT_BLACK : c, i == sel ? c : TFT_BLACK);
                tft.drawCentreString(LABELS[i], bx + bw / 2, y + 4, 1);
            }
            redraw = false;
        }
        if (check(EscPress)) return 2;
        if (check(PrevPress)) sel = (sel + 2) % 3, redraw = true;
        if (check(NextPress)) sel = (sel + 1) % 3, redraw = true;
        if (check(SelPress)) return sel;
        cyberAnimate();
        delay(10);
    }
}
