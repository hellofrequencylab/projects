#include "LedMenu.h"

#include "../cyd_pins.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/utils.h"
#include <globals.h>
#include <math.h>

namespace {

const int PINS[3] = {CYD_LED_R, CYD_LED_G, CYD_LED_B};
// 5 kHz / 8 bit is what Bruce uses for the backlight (TFT_BRIGHT_FREQ/_Bits),
// so ledcAttach shares that timer instead of taking a new one. Never change
// this frequency at runtime: it would retune the backlight too.
constexpr uint32_t LED_FREQ = 5000;
constexpr uint8_t LED_BITS = 8;

bool attached = false;
uint8_t cur[3] = {0, 0, 0};

// Picker state: hue 0-345 in 15-degree steps, 360 = white.
int pickHue = 300;
int pickLevel = 2;
const uint8_t LEVELS[] = {16, 64, 140, 255};
constexpr int LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);

void release() {
    for (int p : PINS) {
        if (attached) ledcDetach(p);
        pinMode(p, OUTPUT);
        digitalWrite(p, CYD_LED_ACTIVE_LOW ? HIGH : LOW); // driven off, not floating
    }
    attached = false;
}

void write(uint8_t r, uint8_t g, uint8_t b) {
    cur[0] = r, cur[1] = g, cur[2] = b;
    if (!r && !g && !b) {
        release();
        return;
    }
    if (!attached) {
        for (int p : PINS) ledcAttach(p, LED_FREQ, LED_BITS);
        attached = true;
    }
    for (int i = 0; i < 3; i++) {
        uint8_t v = (uint16_t)cur[i] * cur[i] / 255; // rough gamma: LEDs look bright early
        if (cur[i] && !v) v = 1;
        ledcWrite(PINS[i], CYD_LED_ACTIVE_LOW ? 255 - v : v);
    }
}

// Hue (degrees) + level -> 8-bit RGB. hue >= 360 means white.
void hueRgb(int hue, uint8_t level, uint8_t *rgb) {
    if (hue >= 360) {
        rgb[0] = rgb[1] = rgb[2] = level;
        return;
    }
    hue = ((hue % 360) + 360) % 360;
    int f = (hue % 60) * 255 / 60;
    int r, g, b;
    switch (hue / 60) {
        case 0: r = 255, g = f, b = 0; break;
        case 1: r = 255 - f, g = 255, b = 0; break;
        case 2: r = 0, g = 255, b = f; break;
        case 3: r = 0, g = 255 - f, b = 255; break;
        case 4: r = f, g = 0, b = 255; break;
        default: r = 255, g = 0, b = 255 - f; break;
    }
    rgb[0] = r * level / 255, rgb[1] = g * level / 255, rgb[2] = b * level / 255;
}

uint16_t to565(const uint8_t *c) { return ((c[0] & 0xF8) << 8) | ((c[1] & 0xFC) << 3) | (c[2] >> 3); }

// Layout shared by the three LED screens: a glowing orb on the left that
// mirrors the LED, text on the right.
constexpr int ORB_X = 80, ORB_Y = 128, ORB_R = 52;

void drawFrame(const char *title, const char *hint1, const char *hint2) {
    drawMainBorderWithTitle(title);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(hint1, 160, 178, 1);
    tft.drawString(hint2, 160, 190, 1);
    tft.drawString("ESC        exit", 160, 202, 1);
    tft.drawCircle(ORB_X, ORB_Y, ORB_R + 4, cyberHue(300));
    tft.drawCircle(ORB_X, ORB_Y, ORB_R + 6, cyberHue(180));
}

// Draw the orb in the LED's colour, scaled so a dim LED still reads on screen.
void drawOrb(const uint8_t *c) {
    uint8_t m = max(c[0], max(c[1], c[2]));
    uint8_t s[3] = {0, 0, 0};
    if (m)
        for (int i = 0; i < 3; i++) s[i] = (uint16_t)c[i] * 255 / m * (128 + m / 2) / 255;
    tft.fillCircle(ORB_X, ORB_Y, ORB_R, to565(s));
    // a little highlight, so it reads as a glowing bead
    tft.fillCircle(ORB_X - ORB_R / 3, ORB_Y - ORB_R / 3, ORB_R / 6, m ? TFT_WHITE : 0x2104);
}

void drawRgbText(const uint8_t *c) {
    tft.setTextSize(2);
    const char *names[3] = {"R ", "G ", "B "};
    const int hues[3] = {0, 120, 240};
    for (int i = 0; i < 3; i++) {
        tft.setTextColor(cyberHue(hues[i]), bruceConfig.bgColor);
        char buf[8];
        snprintf(buf, sizeof(buf), "%s%3d", names[i], c[i]);
        tft.drawString(buf, 160, 96 + i * 20, 1);
    }
}

void colourPicker() {
    drawFrame("COLOUR", "NEXT/PREV  hue", "SEL        brightness");
    const int bx = 160, by = 52, bw = 144, bh = 14;
    for (int i = 0; i < bw; i++) tft.drawFastVLine(bx + i, by, bh, cyberHue(i * 360 / bw));
    bool redraw = true;
    while (true) {
        if (redraw) {
            redraw = false;
            uint8_t c[3];
            hueRgb(pickHue, LEVELS[pickLevel], c);
            write(c[0], c[1], c[2]);
            drawOrb(c);
            drawRgbText(c);
            // hue marker under the bar ("W" at the end for white)
            tft.fillRect(bx - 4, by + bh + 1, bw + 12, 8, bruceConfig.bgColor);
            int mx = pickHue >= 360 ? bx + bw + 2 : bx + pickHue * bw / 360;
            tft.fillTriangle(mx, by + bh + 1, mx - 3, by + bh + 7, mx + 3, by + bh + 7, TFT_WHITE);
            tft.fillRect(bx, 80, bw, 10, bruceConfig.bgColor);
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            tft.drawString(
                (pickHue >= 360 ? String("white") : String(pickHue) + " deg") + "  level " +
                    String(pickLevel + 1) + "/" + String(LEVEL_COUNT),
                bx, 80, 1
            );
        }
        if (check(NextPress)) pickHue = (pickHue + 15) % 375, redraw = true;
        if (check(PrevPress)) pickHue = (pickHue + 360) % 375, redraw = true;
        if (check(SelPress)) pickLevel = (pickLevel + 1) % LEVEL_COUNT, redraw = true;
        if (check(EscPress)) break; // the picked colour stays on
        delay(15);
    }
}

// mode 0 = rainbow cycle, 1 = breathe (on the picker's hue)
void animate(int mode) {
    const int SPEEDS[] = {2, 5, 10, 20};
    int speed = 1;
    if (mode == 0) drawFrame("RAINBOW", "NEXT/PREV  speed", "");
    else drawFrame("BREATHE", "NEXT/PREV  hue", "SEL        speed");
    uint32_t last = 0;
    float t = 0;
    while (true) {
        uint32_t now = millis();
        if (now - last >= 30) {
            t += (now - last) * SPEEDS[speed] / 1000.0f;
            last = now;
            uint8_t c[3];
            if (mode == 0) {
                hueRgb((int)(t * 36) % 360, LEVELS[pickLevel], c);
            } else {
                float k = (1 - cosf(t * 1.2f)) / 2; // 0..1
                hueRgb(pickHue, max(1, (int)(LEVELS[pickLevel] * k)), c); // never 0: that releases the pins
            }
            write(c[0], c[1], c[2]);
            drawOrb(c);
            drawRgbText(c);
        }
        if (mode == 0) {
            if (check(NextPress)) speed = min(speed + 1, 3);
            if (check(PrevPress)) speed = max(speed - 1, 0);
        } else {
            if (check(NextPress)) pickHue = (pickHue + 15) % 375;
            if (check(PrevPress)) pickHue = (pickHue + 360) % 375;
            if (check(SelPress)) speed = (speed + 1) % 4;
        }
        if (check(EscPress)) break;
        delay(5);
    }
    write(0, 0, 0); // animations can't run in the background
}

struct Preset {
    const char *name;
    uint8_t r, g, b;
};
const Preset PRESETS[] = {
    {"Hot pink",      255, 20,  147},
    {"Cyber cyan",    0,   255, 255},
    {"Unicorn violet", 160, 0,  255},
    {"Toxic green",   80,  255, 0  },
    {"Sunset orange", 255, 90,  0  },
};

} // namespace

void cydLedSet(uint8_t r, uint8_t g, uint8_t b) { write(r, g, b); }

void LedMenu::optionsMenu() {
    int idx = 0;
    while (true) {
        bool lit = cur[0] || cur[1] || cur[2];
        options = {
            {"Colour picker", colourPicker        },
            {"Rainbow cycle", []() { animate(0); }},
            {"Breathe",       []() { animate(1); }},
        };
        for (const Preset &p : PRESETS) options.push_back({p.name, [&p]() { write(p.r, p.g, p.b); }});
        options.push_back({lit ? "Off" : "Off (is off)", []() { write(0, 0, 0); }});
        addOptionToMainMenu();
        idx = loopOptions(options, MENU_TYPE_SUBMENU, "LED", idx);
        if (idx < 0 || returnToMenu) break;
    }
}

void LedMenu::drawIcon(float scale) {
    clearIconArea();
    // A 5 mm LED: rainbow dome, two legs.
    int r = scale * 18;
    int cx = iconCenterX, cy = iconCenterY - scale * 8;
    for (int i = r; i > 0; i -= max(1, (int)(scale * 3))) tft.fillCircle(cx, cy, i, cyberHue(300 + (r - i) * 8));
    tft.fillRect(cx - r, cy, 2 * r + 1, r / 2, cyberHue(300));
    tft.drawFastHLine(cx - r - 3, cy + r / 2, 2 * r + 7, bruceConfig.priColor);
    tft.drawFastVLine(cx - r / 2, cy + r / 2, scale * 22, bruceConfig.priColor);
    tft.drawFastVLine(cx + r / 2, cy + r / 2, scale * 16, bruceConfig.priColor);
}

static uint32_t ledCallback(cmd *c) {
    Command cmd(c);
    String a = cmd.getArgument("r").getValue();
    if (a == "off") {
        write(0, 0, 0);
        serialDevice->println("LED off");
        return true;
    }
    int v[3] = {a.toInt(), cmd.getArgument("g").getValue().toInt(), cmd.getArgument("b").getValue().toInt()};
    for (int &x : v) x = constrain(x, 0, 255);
    write(v[0], v[1], v[2]);
    serialDevice->printf("LED R%d G%d B%d (pins %d/%d/%d)\n", v[0], v[1], v[2], CYD_LED_R, CYD_LED_G, CYD_LED_B);
    return true;
}

void createLedCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("led", ledCallback);
    cmd.addPosArg("r", "0");
    cmd.addPosArg("g", "0");
    cmd.addPosArg("b", "0");
}
