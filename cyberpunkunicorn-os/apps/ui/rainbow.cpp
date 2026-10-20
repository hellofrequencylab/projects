#include "rainbow.h"

#include <globals.h>

uint16_t cyberHue(int hue) {
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

uint16_t cyberItemColor(int index, int count) {
    if (count <= 0) count = 1;
    // Start at hot pink and walk the wheel; never fewer than 30 degrees apart.
    int step = max(30, 360 / count);
    return cyberHue(300 + index * step);
}

int cyberPhase() { return (int)((millis() / 25) % 360); }

void cyberRainbowFrame(int x, int y, int w, int h, int phase) {
    if (w <= 0 || h <= 0) return;
    const int perim = 2 * (w + h);
    // Draw in short runs so it stays cheap over SPI.
    const int run = 6;
    for (int i = 0; i < w; i += run) {
        int len = min(run, w - i);
        tft.drawFastHLine(x + i, y, len, cyberHue(phase + i * 360 / perim));
        tft.drawFastHLine(x + w - i - len, y + h - 1, len, cyberHue(phase + (w + h + i) * 360 / perim));
    }
    for (int j = 0; j < h; j += run) {
        int len = min(run, h - j);
        tft.drawFastVLine(x + w - 1, y + j, len, cyberHue(phase + (w + j) * 360 / perim));
        tft.drawFastVLine(x, y + h - j - len, len, cyberHue(phase + (2 * w + h + j) * 360 / perim));
    }
}

void cyberRainbowHLine(int x, int y, int w, int phase) {
    const int run = 4;
    for (int i = 0; i < w; i += run)
        tft.drawFastHLine(x + i, y, min(run, w - i), cyberHue(phase + i * 360 / max(w, 1)));
}

void cyberRainbowFill(int x, int y, int w, int h, int phase) {
    for (int i = 0; i < w; i++) tft.drawFastVLine(x + i, y, h, cyberHue(phase + i * 2));
}

int cyberRainbowText(const String &text, int x, int y, int phase, uint16_t bg) {
    const int cw = LW * tft.getTextSize();
    for (unsigned i = 0; i < text.length(); i++) {
        tft.setTextColor(cyberHue(phase + i * 25), bg);
        tft.drawString(String(text[i]), x + i * cw, y, 1);
    }
    return cw * text.length();
}
