#include "unicorn_boot.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "unicorn_sprite.h"
#include <globals.h>
#include <new>

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
// Finale phases, as fractions of the duration (design/components/device/BootScene.jsx):
// gallop, tremble during the loader's stall, blast off over the sun, rainbow flood.
constexpr float P_TREMBLE = 0.46f, P_BLAST = 0.6f, P_WASH = 0.86f;
constexpr int JET = 40;    // jet ribbon points (ring)
constexpr int PLUME = 96;  // spray blobs, recycled
const uint16_t STAR_C[4] = {0xFFFF, 0x07FF, 0xFAD9, 0xFEE7};

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

// Unicorn at any scale, squashed horizontally (sx < 1) as it banks away. cx = its middle.
void drawUnicornAt(TFT_eSprite *s, float cx, float top, float sc, float sx, int frame, int t) {
    const uint8_t *px = uni_frames[frame];
    const int w = max(1, (int)ceilf(sc * sx)), h = max(1, (int)ceilf(sc));
    for (int y = 0; y < UNI_H; y++) {
        uint16_t mane = hue565(y * 18 + t * 12);
        for (int x = 0; x < UNI_W; x++) {
            uint8_t idx = px[y * UNI_W + x];
            if (idx == UNI_TRANSPARENT) continue;
            s->fillRect(lroundf(cx + (x - 20) * sc * sx), lroundf(top + y * sc), w, h, idx == UNI_RAINBOW ? mane : uni_palette[idx]);
        }
    }
}

// 4-point pixel sparkle; big ones get a white core.
void star(TFT_eSprite *d, int x, int y, int r, uint16_t c) {
    d->drawFastHLine(x - r, y, 2 * r + 1, c);
    d->drawFastVLine(x, y - r, 2 * r + 1, c);
    if (r > 2) d->fillRect(x - 1, y - 1, 3, 3, TFT_WHITE);
}

int snap3(float v) { return (int)lroundf(v / 3) * 3; }

struct JetPt {
    int16_t x, y;
    float sc;
};
struct Blob {
    float x, y, vx, vy, s, g;
    uint8_t max, hue;
};

// The rainbow jet behind the rocketing unicorn: a ribbon that fattens with age,
// plus chunky blobs that spray back at the viewer and swell.
struct Finale {
    JetPt jet[JET];
    Blob plume[PLUME];
    int jetN = 0, jetHead = 0, blobN = 0, blobNext = 0;

    void emit(float x, float y, float sc, float n) {
        jet[jetHead] = {(int16_t)x, (int16_t)y, sc};
        jetHead = (jetHead + 1) % JET;
        jetN = min(jetN + 1, JET);
        for (int i = 0, burst = 3 + (int)(n * 5); i < burst; i++) {
            Blob &b = plume[blobNext];
            blobNext = (blobNext + 1) % PLUME;
            blobN = min(blobN + 1, PLUME);
            b = {x + ((int)(rnd() % 5) - 2) * sc, y, ((int)(rnd() % 13) - 6) * 0.45f, ((int)(rnd() % 11) - 2) * 0.45f,
                 max(1.5f, sc), 1.05f + (rnd() % 5) * 0.01f, (uint8_t)(24 + rnd() % 24), (uint8_t)(rnd() % 255)};
        }
    }

    void draw(TFT_eSprite *s, int t) {
        for (int j = 0; j < jetN; j++) { // oldest first
            const JetPt &p = jet[(jetHead - jetN + j + JET) % JET];
            float w = min(60.0f, p.sc * 2 * (1 + (jetN - j) * 0.12f));
            for (int b = 0; b < 6; b++)
                s->fillRect(snap3(p.x - w), snap3(p.y + (b - 3) * w * 0.9f), snap3(w * 2) + 3, (int)ceilf(w * 0.9f) + 1,
                            hue565(b * 60 - t * 10));
        }
        for (int i = 0; i < blobN; i++) {
            Blob &q = plume[i];
            float z = q.s / 3;
            q.x += q.vx * z;
            q.y += q.vy * z;
            if (q.s < q.max) q.s *= q.g;
            int sz = max(3, snap3(q.s));
            s->fillRect(snap3(q.x), snap3(q.y), sz, sz, hue565(q.hue * 360 / 255 - t * 6));
            if ((i + t) % 7 == 0) star(s, snap3(q.x) + 1, snap3(q.y) + 1, 2 + i % 3, STAR_C[(i + (t >> 2)) % 4]);
        }
    }
};

// The flood: a diagonal rainbow wipe from the bottom that eats the screen, in
// 6 px cells. Scene rows go into the frame buffer, the rest straight to the panel.
void washCell(TFT_eSprite *s, int x, int y, int sz, uint16_t c) {
    const int a = max(y, TITLE_H), b = min(y + sz, TITLE_H + SCENE_H); // rows in the scene
    if (a < b) s->fillRect(x, a - TITLE_H, sz, b - a, c);
    if (y < a) tft.fillRect(x, y, sz, min(a, y + sz) - y, c);
    if (y + sz > max(a, b)) tft.fillRect(x, max(y, b), sz, y + sz - max(y, b), c);
}

void drawWash(TFT_eSprite *s, float w, int t) {
    const int H = tft.height(), W = tft.width();
    const float front = w * 300 - 30;
    for (int y = 0; y < H; y += 6)
        for (int x = 0; x < W; x += 6)
            if (H - y <= front + sinf((x + t * 6) / 16.0f) * 14) washCell(s, x, y, 6, hue565((int)((x + y) * 1.2f) - t * 14));
    for (int i = 0; i < 40; i++) {
        int x = (i * 97 + t * 13) % W, y = (i * 61 + t * 7) % H;
        if (H - y < front && (i + (t >> 1)) % 3) {
            int r = 2 + i % 4;
            if (y - r >= TITLE_H && y + r < TITLE_H + SCENE_H) star(s, x, y - TITLE_H, r, STAR_C[i % 4]);
        }
    }
}

// Tagline over the flood, with the title's cyan/magenta shadow.
void drawWashTagline() {
    const String msg = BOOT_TAGLINE;
    const int sz = msg.length() * 12 > 300 ? 1 : 2, x0 = (tft.width() - msg.length() * 6 * sz) / 2, y0 = 112;
    tft.setTextSize(sz);
    const uint16_t col[3] = {rgb(0, 255, 255), rgb(255, 0, 160), TFT_WHITE};
    for (int i = 0; i < 3; i++) {
        tft.setTextColor(col[i]);
        tft.drawString(msg, x0 + (i == 0 ? -2 : i == 1 ? 2 : 0), y0, 1);
    }
    tft.setTextSize(1);
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

// Progress bar that goes jerkily, stalls while the unicorn trembles, then
// sails straight past 100% as it blasts off.
void drawLoading(float k, int t, int &lastMsg, int &lastPct) {
    const int W = tft.width(), y = TITLE_H + SCENE_H;
    const int barX = 16, barY = y + 6, barW = W - 32, barH = 10;

    int pct;
    if (k < 0.4f) pct = (int)(k / 0.4f * 64);                // brisk
    else if (k < P_BLAST) pct = 64 + (int)((k - 0.4f) * 14);  // the famous stall
    else pct = min(137, 67 + (int)((k - P_BLAST) / (P_WASH - P_BLAST) * 70));

    int fill = barW - 2;
    int done = pct >= 100 ? fill : fill * pct / 100;
    tft.drawRect(barX, barY, barW, barH, rgb(0, 255, 255));
    for (int x = 0; x < done; x++) tft.drawFastVLine(barX + 1 + x, barY + 1, barH - 2, hue565(x * 2 - t * 10));

    // Message ids: 0..LOADING_COUNT-1 the jokes, then the finale lines.
    static const char *const FINALE[] = {
        "Something is brewing...", "Uh oh. Rainbow pressure rising...", "Banking left...", "Venting rainbows...",
        "BLAST OFF!!!"};
    int msg;
    if (k >= P_BLAST) msg = LOADING_COUNT + (k < P_BLAST + 0.03f ? 2 : pct >= 100 ? 4 : 3);
    else if (k >= P_TREMBLE) msg = LOADING_COUNT + ((t >> 3) & 1);
    else msg = (int)(k * 1.8f * LOADING_COUNT) % LOADING_COUNT;
    const bool hot = k >= P_TREMBLE;
    if (msg != lastMsg || pct != lastPct || hot) {
        tft.fillRect(0, barY + barH + 3, W, tft.height() - (barY + barH + 3), TFT_BLACK);
        tft.setTextSize(1);
        tft.setTextColor(hot ? hue565(t * 20) : TFT_WHITE, TFT_BLACK);
        tft.drawString(msg >= LOADING_COUNT ? FINALE[msg - LOADING_COUNT] : LOADING[msg], barX, barY + barH + 6, 1);
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
    Finale *fin = new (std::nothrow) Finale(); // ~3.7 KB, only for the boot

    tft.fillScreen(TFT_BLACK);
    Spark sparks[SPARKS] = {};
    int lastMsg = -1, lastPct = -1;
    const uint32_t start = millis();
    int t = 0;

    while (millis() - start < durationMs) {
        uint32_t frameStart = millis();
        // Without the finale's RAM the old loop plays: gallop and bob.
        const float k = fin ? (float)(millis() - start) / durationMs : 0.4f * (millis() - start) / durationMs;
        // The unicorn gallops in, then bobs in place while the grid races by.
        int ux = -110 + min(t * 6, 170);
        int uy = 22 + (int)(3.0f * sinf(t / 2.0f));
        int frame = (t / 4) % UNI_FRAMES, shake = 0;
        bool trembling = k >= P_TREMBLE && k < P_BLAST;
        if (trembling) {
            float n = (k - P_TREMBLE) / (P_BLAST - P_TREMBLE), amp = 1 + n * 4;
            ux += (int)(((int)(rnd() % 3) - 1) * amp);
            uy = 22 + (int)(((int)(rnd() % 3) - 1) * amp);
            frame = 0;
            shake = n > 0.6f;
        }

        drawSky(s, t);
        drawGrid(s, t);
        if (k < P_BLAST) drawTrail(s, ux, uy, t);
        if (k < P_TREMBLE) drawSparks(s, sparks, ux, uy, t);
        if (k >= P_BLAST) {
            // Bank left (squash) to face the horizon, then rocket up and away over
            // the sun, shrinking into the distance, spraying rainbow at the viewer.
            float n = min(1.0f, (k - P_BLAST) / (P_WASH - P_BLAST));
            float cx = 120, top = 22, sc = 3, sx = 1;
            if (n < 0.16f) {
                float tn = n / 0.16f;
                sx = 1 - 0.55f * tn;
                top = 22 - tn * 6;
                shake = 2;
                frame = 0;
            } else {
                float m = (n - 0.16f) / 0.84f, e = 1 - (1 - m) * (1 - m);
                sx = 0.45f;
                sc = 3 - 2.55f * e;
                cx = 120 + (226 - 120) * e;
                top = 16 + 10 * e - sinf(m * PI) * 18 - sc * 6;
                frame = (t / 2) % UNI_FRAMES;
                shake = sc > 1.5f ? 3 : 1;
            }
            if (n >= 0.1f) fin->emit(cx, top + 18 * sc, sc, n);
            if (n < 0.97f) drawUnicornAt(s, cx, top, sc, sx, frame, t);
            else star(s, (int)cx, (int)top + 6, 6, STAR_C[t % 4]);
            fin->draw(s, t);
        } else {
            drawUnicorn(s, ux, uy, frame, t);
        }
        if (trembling && (t >> 2) % 2) { // "!!" and a bead of sweat
            s->setTextSize(2);
            s->setTextColor(rgb(255, 0, 200));
            s->drawString("!!", ux + 34 * UNI_SCALE, uy - 18, 1);
            s->setTextSize(1);
            s->fillRect(ux + 26 * UNI_SCALE + t % 3, uy + 4 + (t % 6) * 2, 2, 3, rgb(0, 255, 255));
        }

        const bool washing = k >= P_WASH - 0.04f;
        const float w = min(1.0f, (k - P_WASH + 0.04f) / 0.12f);
        if (t % 3 == 0 || shake) drawTitle(t);
        drawLoading(k, t, lastMsg, lastPct);
        if (washing) drawWash(s, w, t);
        int dx = shake ? (int)(rnd() % (2 * shake + 1)) - shake : 0, dy = shake ? (int)(rnd() % (2 * shake + 1)) - shake : 0;
        buf.pushSprite(dx, TITLE_H + dy);
        if (washing && w >= 1) drawWashTagline();

        if (check(AnyKeyPress)) break;
        t++;
        uint32_t spent = millis() - frameStart;
        if (spent < FRAME_MS) delay(FRAME_MS - spent);
    }

    delete fin;
    buf.deleteSprite();
    return true;
}
