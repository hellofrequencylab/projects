#include "LightMenu.h"

#include "../ui/cyber_icon.h"

#include "../cyd_pins.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/utils.h"
#include <globals.h>

namespace {

struct Reading {
    int raw; // 0-4095
    int mv;
    int pct; // 0 = dark, 100 = bright
};

Reading readLight() {
    // 0 dB attenuation (0-~1 V full scale): the CYD's LDR divider sits low,
    // so 11 dB would squash it into the bottom few percent.
    analogSetPinAttenuation(CYD_LDR, ADC_0db);
    uint32_t raw = 0, mv = 0;
    for (int i = 0; i < 8; i++) raw += analogRead(CYD_LDR), mv += analogReadMilliVolts(CYD_LDR);
    Reading r;
    r.raw = raw / 8;
    r.mv = mv / 8;
    r.pct = r.raw * 100 / 4095;
    if (CYD_LDR_BRIGHT_LOW) r.pct = 100 - r.pct;
    return r;
}

const char *band(int pct) {
    if (pct < 10) return "midnight";
    if (pct < 30) return "dim";
    if (pct < 60) return "indoor";
    if (pct < 85) return "bright";
    return "SUNBEAM";
}

// Bar / graph colour: deep violet when dark, through the rainbow, to hot pink.
uint16_t levelColor(int pct) { return cyberHue(270 - pct * 330 / 100); }

constexpr int X = 16, W = 288;
constexpr int BAR_Y = 100, BAR_H = 22;
constexpr int G_Y = 130, G_H = 70, STEP = 2, N = W / STEP;

void meter() {
    drawMainBorderWithTitle("LIGHT");
    cyberRainbowFrame(X - 2, BAR_Y - 2, W + 4, BAR_H + 4, 0);
    cyberRainbowFrame(X - 2, G_Y - 2, W + 4, G_H + 4, 180);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString("SEL reset min/max    ESC exit", X, G_Y + G_H + 6, 1);

    uint8_t hist[N] = {};
    int count = 0, lo = 4095, hi = 0, shownBar = -1;
    uint32_t last = 0;
    while (true) {
        if (millis() - last >= 120) {
            last = millis();
            Reading r = readLight();
            lo = min(lo, r.raw), hi = max(hi, r.raw);

            // Big percentage in rainbow letters, band name beside it.
            tft.setTextSize(4);
            char buf[8];
            snprintf(buf, sizeof(buf), "%3d%%", r.pct);
            cyberRainbowText(buf, X, 44, cyberPhase(), bruceConfig.bgColor);
            tft.setTextSize(2);
            tft.setTextColor(levelColor(r.pct), bruceConfig.bgColor);
            tft.drawString(String(band(r.pct)) + "        ", X + 112, 46, 1);
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            tft.drawString("raw " + String(r.raw) + "  " + String(r.mv) + " mV   ", X + 112, 68, 1);
            tft.drawString("min " + String(lo) + "  max " + String(hi) + "   ", X + 112, 78, 1);
            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString("GPIO" + String(CYD_LDR) + " LDR", X + 112, 88, 1);

            // Bar: rainbow up to the level, dark after it. Only redraw the change.
            int fill = r.pct * W / 100;
            if (fill != shownBar) {
                for (int i = 0; i < W; i++)
                    tft.drawFastVLine(X + i, BAR_Y, BAR_H, i < fill ? levelColor(i * 100 / W) : 0x1082);
                shownBar = fill;
            }

            // History: newest on the right, one column per sample.
            memmove(hist, hist + 1, N - 1);
            hist[N - 1] = r.pct;
            if (count < N) count++;
            for (int i = N - count; i < N; i++) {
                int h = hist[i] * (G_H - 1) / 100 + 1;
                int x = X + i * STEP;
                tft.drawFastVLine(x, G_Y, G_H - h, bruceConfig.bgColor);
                tft.drawFastVLine(x, G_Y + G_H - h, h, levelColor(hist[i]));
            }
        }
        if (check(SelPress)) lo = 4095, hi = 0;
        if (check(EscPress)) break;
        delay(10);
    }
}

} // namespace

void LightMenu::optionsMenu() { meter(); }

void LightMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

static uint32_t lightCallback(cmd *c) {
    Reading r = readLight();
    char buf[80];
    snprintf(buf, sizeof(buf), "Light %d%% (%s)  raw %d  %d mV  GPIO%d", r.pct, band(r.pct), r.raw, r.mv, CYD_LDR);
    serialDevice->println(String(buf));
    return true;
}

void createLightCommand(SimpleCLI *cli) { cli->addCommand("light", lightCallback); }
