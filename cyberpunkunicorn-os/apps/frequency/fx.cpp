#include "fx.h"

#include "../boot/unicorn_sprite.h"
#include "../ui/rainbow.h"
#include <globals.h>
#include <math.h>

namespace freq {

void fxWave(int x, int y, int w, int h, int t) {
    const int mid = y + h / 2, amp = h / 2 - 3;
    for (int i = 0; i < w; i++) {
        tft.drawFastVLine(x + i, y, h, bruceConfig.bgColor);
        for (int k = 0; k < 3; k++) {
            float f = 0.045f + k * 0.03f;
            float a = amp * (0.55f + 0.45f * sinf(t * 0.05f + k * 2.1f));
            int py = mid + (int)(a * sinf(i * f + t * (0.18f + k * 0.07f) + k * 2.0f));
            tft.fillRect(x + i, py - 1, 1, 3, cyberHue(i * 2 + t * 6 + k * 120));
        }
    }
}

void fxBurst(int cx, int cy) {
    const int maxR = tftWidth > tftHeight ? tftWidth : tftHeight;
    for (int r = 4, f = 0; r < maxR; r += 9, f++) {
        tft.drawCircle(cx, cy, r, cyberHue(r * 3));
        tft.drawCircle(cx, cy, r + 1, cyberHue(r * 3 + 40));
        tft.drawCircle(cx, cy, r + 2, cyberHue(r * 3 + 80));
        for (int k = 0; k < 10; k++) { // confetti
            int px = esp_random() % tftWidth, py = esp_random() % tftHeight;
            tft.fillRect(px, py, 3 + (k & 1) * 2, 3, cyberHue(esp_random() % 360));
        }
        if (r > 30) tft.drawCircle(cx, cy, r - 30, bruceConfig.bgColor); // hollow it out
        delay(14);
    }
}

void fxUnicorn(int x, int y, int scale, int frame, int phase) {
    const uint8_t *px = uni_frames[frame % UNI_FRAMES];
    for (int row = 0; row < UNI_H; row++) {
        uint16_t mane = cyberHue(row * 18 + phase);
        for (int col = 0; col < UNI_W; col++) {
            uint8_t idx = px[row * UNI_W + col];
            if (idx == UNI_TRANSPARENT) continue;
            tft.fillRect(x + col * scale, y + row * scale, scale, scale, idx == UNI_RAINBOW ? mane : uni_palette[idx]);
        }
    }
}

namespace {
void stampAt(int cx, int cy, int r, bool filled, int hue) {
    tft.fillCircle(cx, cy, r, bruceConfig.bgColor);
    tft.drawCircle(cx, cy, r, cyberHue(hue));
    tft.drawCircle(cx, cy, r - 1, cyberHue(hue + 30));
    if (!filled) return;
    tft.fillCircle(cx, cy, r - 4, cyberHue(hue));
    // a little star in the middle
    tft.fillTriangle(cx, cy - r / 2, cx - r / 3, cy + r / 4, cx + r / 3, cy + r / 4, TFT_WHITE);
    tft.fillTriangle(cx, cy + r / 2 - 1, cx - r / 3, cy - r / 6, cx + r / 3, cy - r / 6, TFT_WHITE);
}
} // namespace

void fxStampCard(int x, int y, int w, int required, int from, int to) {
    required = constrain(required, 1, 12);
    to = constrain(to, 0, required);
    from = constrain(from, 0, to);
    const int perRow = required > 6 ? (required + 1) / 2 : required;
    const int cell = min(w / perRow, 44);
    const int r = cell / 2 - 3;
    const int left = x + (w - perRow * cell) / 2;
    auto centre = [&](int i, int &cx, int &cy) {
        cx = left + (i % perRow) * cell + cell / 2;
        cy = y + (i / perRow) * cell + cell / 2;
    };
    for (int i = 0; i < required; i++) {
        int cx, cy;
        centre(i, cx, cy);
        stampAt(cx, cy, r, i < from, i * 360 / required);
    }
    for (int i = from; i < to; i++) {
        int cx, cy;
        centre(i, cx, cy);
        delay(180);
        for (int s = 2; s <= r; s += 3) { // pop: grow, then settle
            tft.fillCircle(cx, cy, s, cyberHue(i * 360 / required + s * 8));
            delay(12);
        }
        stampAt(cx, cy, r, true, i * 360 / required);
        for (int k = 0; k < 8; k++) { // sparkle
            float a = k * 0.785f;
            int sx = cx + (int)((r + 5) * cosf(a)), sy = cy + (int)((r + 5) * sinf(a));
            tft.fillRect(sx - 1, sy - 1, 3, 3, cyberHue(k * 45));
        }
    }
}

void fxCentreText(const String &text, int cy, int size, int phase) {
    tft.setTextSize(size);
    while (size > 1 && (int)text.length() * LW * size > tftWidth - 16) tft.setTextSize(--size);
    int w = text.length() * LW * size;
    cyberRainbowText(text, max(4, (tftWidth - w) / 2), cy - size * 4, phase, bruceConfig.bgColor);
    tft.setTextSize(1);
}

} // namespace freq
