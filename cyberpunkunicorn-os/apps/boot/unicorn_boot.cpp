#include "unicorn_boot.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "unicorn_sprite.h"
#include <globals.h>

#ifndef BOOT_TITLE
#define BOOT_TITLE "CYBERUNICORN 5000"
#endif
#ifndef BOOT_TAGLINE
#define BOOT_TAGLINE "Ready to sparkle"
#endif

namespace {

// Screen bands (full 320x240 panel, including the touch footer).
constexpr int TITLE_H = 34;   // glitch title, drawn straight to the panel
constexpr int SCENE_H = 170;  // sky + grid + unicorn, drawn in an 8-bit frame buffer
constexpr int HORIZON = 110;  // inside the scene buffer
constexpr int UNI_SCALE = 3;  // device pixels per unicorn pixel
constexpr int FRAME_MS = 40;  // ~25 fps
constexpr int SPARKS = 18;

const char *const LOADING[] = {
    "Polishing horn...",
    "Reticulating rainbows...",
    "Downloading more RAM...",
    "Charging glitter capacitors...",
    "Bribing the CC1101...",
    "Untangling the mane...",
    "Installing hooves.dll...",
    "Teaching IR to see colour...",
    "Negotiating with the nRF24...",
    "Overclocking sparkles...",
    "Summoning the PN532 spirit...",
    "Calibrating vibes...",
};
constexpr int LOADING_COUNT = sizeof(LOADING) / sizeof(LOADING[0]);

uint32_t rng = 0xC0FFEE42u;
uint32_t rnd() {
    rng = rng * 1664525u + 1013904223u;
    return rng >> 8;
}

// Hue 0-359 at full saturation/brightness -> RGB565.
uint16_t hue565(int hue) {
    hue %= 360;
    if (hue < 0) hue += 360;
    int sector = hue / 60, f = (hue % 60) * 255 / 60;
    uint8_t r, g, b;
    switch (sector) {
        case 0: r = 255, g = f, b = 0; break;
        case 1: r = 255 - f, g = 255, b = 0; break;
        case 2: r = 0, g = 255, b = f; break;
        case 3: r = 0, g = 255 - f, b = 255; break;
        case 4: r = f, g = 0, b = 255; break;
        default: r = 255, g = 0, b = 255 - f; break;
    }
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

struct Spark {
    int16_t x, y;
    uint8_t life;
};

void drawSky(TFT_eSprite *s, int t) {
    // Purple-to-magenta gradient, banded like an 80s poster.
    for (int y = 0; y < HORIZON; y++) {
        int k = y * 255 / HORIZON;
        s->drawFastHLine(0, y, s->width(), rgb(30 + k / 3, 0, 70 - k / 6));
    }
    // A few stars that blink with time.
    for (int i = 0; i < 24; i++) {
        int x = (i * 97 + 13) % s->width(), y = (i * 53 + 7) % (HORIZON - 30);
        if (((i * 7 + t / 6) % 9) != 0) s->drawPixel(x, y, TFT_WHITE);
    }
    // Striped sun, sitting on the horizon.
    const int cx = 228, cy = HORIZON, r = 46;
    for (int y = cy - r; y < cy; y++) {
        int dy = cy - y;
        int dx = (int)sqrtf((float)(r * r - dy * dy));
        int stripeGap = dy < 22 ? (dy % 6 < 2) : 0; // stripes near the bottom
        if (stripeGap) continue;
        int k = (y - (cy - r)) * 255 / r;
        s->drawFastHLine(cx - dx, y, 2 * dx, rgb(255, 220 - k * 160 / 255, 60 + k * 120 / 255));
    }
}

void drawGrid(TFT_eSprite *s, int t) {
    const int W = s->width(), depth = SCENE_H - HORIZON;
    s->fillRect(0, HORIZON, W, depth, rgb(12, 0, 28));
    // Horizontal lines rush toward the viewer.
    const int N = 9;
    float phase = (t % 24) / 24.0f;
    for (int i = 0; i < N; i++) {
        float z = (i + phase) / N;
        int y = HORIZON + (int)(z * z * depth);
        s->drawFastHLine(0, y, W, rgb(0, 160 + (int)(z * 95), 255));
    }
    // Vanishing-point lines.
    for (int i = -11; i <= 11; i++) s->drawLine(W / 2 + i * 9, HORIZON, W / 2 + i * 64, SCENE_H - 1, rgb(255, 0, 200));
    s->drawFastHLine(0, HORIZON, W, rgb(255, 120, 255));
}

void drawTrail(TFT_eSprite *s, int ux, int uy, int t) {
    // Six rainbow bands streaming out behind the unicorn, waving.
    const int top = uy + 15 * UNI_SCALE;
    for (int x = 0; x < ux + 6 * UNI_SCALE; x++) {
        int wave = (int)(4.0f * sinf((x + t * 6) / 11.0f));
        for (int band = 0; band < 6; band++) {
            s->drawFastVLine(x, top + band * 4 + wave, 4, hue565(band * 60 - t * 8));
        }
    }
}

void drawUnicorn(TFT_eSprite *s, int ux, int uy, int frame, int t) {
    const uint8_t *px = uni_frames[frame];
    for (int y = 0; y < UNI_H; y++) {
        uint16_t mane = hue565(y * 18 + t * 12);
        for (int x = 0; x < UNI_W; x++) {
            uint8_t idx = px[y * UNI_W + x];
            if (idx == UNI_TRANSPARENT) continue;
            uint16_t c = idx == UNI_RAINBOW ? mane : uni_palette[idx];
            s->fillRect(ux + x * UNI_SCALE, uy + y * UNI_SCALE, UNI_SCALE, UNI_SCALE, c);
        }
    }
}

void drawSparks(TFT_eSprite *s, Spark *sp, int ux, int uy, int t) {
    for (int i = 0; i < SPARKS; i++) {
        Spark &k = sp[i];
        if (k.life == 0) {
            k.x = (int)(rnd() % (uint32_t)(ux + 40));
            k.y = uy + 30 + (int)(rnd() % 50) - 25;
            k.life = 6 + rnd() % 10;
        }
        k.x -= 3;
        k.life--;
        uint16_t c = hue565(i * 40 + t * 10);
        int a = k.life > 4 ? 2 : 1;
        s->drawFastHLine(k.x - a, k.y, 2 * a + 1, c);
        s->drawFastVLine(k.x, k.y - a, 2 * a + 1, c);
    }
}

// Title with a chromatic-aberration shadow, per-letter rainbow and the odd glitch.
void drawTitle(int t) {
    const int W = tft.width();
    tft.fillRect(0, 0, W, TITLE_H, TFT_BLACK);
    const String title = BOOT_TITLE;
    uint8_t size = 3;
    tft.setTextSize(size);
    if (tft.textWidth(title, 1) > W - 4) size = 2;
    tft.setTextSize(size);
    const int cw = 6 * size, tmW = 12, w = cw * title.length() + tmW;
    int x0 = (W - w) / 2, y0 = (TITLE_H - 8 * size) / 2;
    bool glitch = (rnd() % 7) == 0;
    int jitter = glitch ? (int)(rnd() % 9) - 4 : 0;

    tft.setTextColor(rgb(0, 255, 255));
    tft.drawString(title, x0 - 2 + jitter, y0, 1);
    tft.setTextColor(rgb(255, 0, 160));
    tft.drawString(title, x0 + 2 - jitter, y0 + (glitch ? 1 : 0), 1);
    for (unsigned i = 0; i < title.length(); i++) {
        tft.setTextColor(hue565(i * 22 - t * 9));
        tft.drawString(String(title[i]), x0 + i * cw, y0, 1);
    }
    tft.setTextSize(1); // superscript TM (the font has no trademark glyph)
    tft.setTextColor(TFT_WHITE);
    tft.drawString("TM", x0 + cw * title.length() + 1, y0, 1);
    tft.setTextSize(size);
    if (glitch) { // a slice of the title slides sideways for one frame
        int sy = y0 + (int)(rnd() % (8 * size));
        tft.fillRect(0, sy, W, 2 + rnd() % 3, hue565(rnd() % 360));
    }
}

// Progress bar that goes jerkily, stalls, then sails straight past 100%.
void drawLoading(uint32_t elapsed, uint32_t duration, int t, int &lastMsg, int &lastPct) {
    const int W = tft.width(), y = TITLE_H + SCENE_H;
    const int barX = 16, barY = y + 6, barW = W - 32, barH = 10;

    float k = (float)elapsed / (float)duration;
    int pct;
    if (k < 0.45f) pct = (int)(k / 0.45f * 64);            // brisk
    else if (k < 0.62f) pct = 64 + (int)((k - 0.45f) * 18); // the famous stall
    else pct = 67 + (int)((k - 0.62f) / 0.38f * 70);        // then 137%

    int fill = barW - 2;
    int done = pct >= 100 ? fill : fill * pct / 100;
    tft.drawRect(barX, barY, barW, barH, rgb(0, 255, 255));
    for (int x = 0; x < done; x++) tft.drawFastVLine(barX + 1 + x, barY + 1, barH - 2, hue565(x * 2 - t * 10));

    int msg = pct >= 100 ? LOADING_COUNT : (int)(k * 1.8f * LOADING_COUNT) % LOADING_COUNT;
    if (msg != lastMsg || pct != lastPct) {
        tft.fillRect(0, barY + barH + 3, W, 240 - (barY + barH + 3), TFT_BLACK);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        String text = msg == LOADING_COUNT ? String(BOOT_TAGLINE) : String(LOADING[msg]);
        tft.drawString(text, barX, barY + barH + 6, 1);
        String p = pct >= 100 ? String(pct) + "% (oops)" : String(pct) + "%";
        tft.setTextColor(pct >= 100 ? hue565(t * 20) : rgb(255, 0, 200), TFT_BLACK);
        tft.drawRightString(p, barX + barW, barY + barH + 6, 1);
        lastMsg = msg;
        lastPct = pct;
    }
}

} // namespace

bool unicornBootScene(uint32_t durationMs) {
    tft_sprite buf(&tft);
    buf.setColorDepth(8);
    if (!buf.createSprite(tft.width(), SCENE_H)) return false;
    TFT_eSprite *s = buf.nativeSprite();

    tft.fillScreen(TFT_BLACK);
    Spark sparks[SPARKS] = {};
    int lastMsg = -1, lastPct = -1;
    const uint32_t start = millis();
    int t = 0;

    while (millis() - start < durationMs) {
        uint32_t frameStart = millis();
        // The unicorn gallops in, then bobs in place while the grid races by.
        int enter = min(t * 6, 170);
        int ux = -110 + enter;
        int uy = 22 + (int)(3.0f * sinf(t / 2.0f));
        int frame = (t / 4) % UNI_FRAMES;

        drawSky(s, t);
        drawGrid(s, t);
        drawTrail(s, ux, uy, t);
        drawSparks(s, sparks, ux, uy, t);
        drawUnicorn(s, ux, uy, frame, t);
        buf.pushSprite(0, TITLE_H);

        if (t % 3 == 0) drawTitle(t);
        drawLoading(millis() - start, durationMs, t, lastMsg, lastPct);

        if (check(AnyKeyPress)) break;
        t++;
        uint32_t spent = millis() - frameStart;
        if (spent < FRAME_MS) delay(FRAME_MS - spent);
    }

    buf.deleteSprite();
    return true;
}
