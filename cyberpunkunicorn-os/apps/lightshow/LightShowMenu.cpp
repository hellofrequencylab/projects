#include "LightShowMenu.h"

#include "../hat/hat_probe.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/utils.h"
#include "scenes.h"
#include <Preferences.h>
#include <esp32-hal-rmt.h>
#include <globals.h>
#include <math.h>

using namespace lightshow;

namespace {

// Brightness steps. Kept low for USB power: 64 LEDs at full white draw ~3.8 A.
const float LEVELS[] = {0.04f, 0.08f, 0.15f, 0.25f, 0.4f};
constexpr int LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);

// Output-capable pins a panel can realistically use on the CYD + hat. 22/27 are
// the Grove/CN1 pins the hat modules share, so they need DIP 1-5 OFF.
const int PIN_CHOICES[] = {27, 22, 26, 4, 16, 17, -1};
constexpr int PIN_CHOICE_COUNT = sizeof(PIN_CHOICES) / sizeof(PIN_CHOICES[0]);

struct Settings {
    int pin = 27; // -1 = screen only
    bool zigzag = false;
    int rotate = 0; // 0/90/180/270
    int level = 1;
    int index = 0;

    void load() {
        Preferences p;
        p.begin("lightshow", true);
        pin = p.getInt("pin", pin);
        zigzag = p.getBool("zigzag", zigzag);
        rotate = p.getInt("rotate", rotate);
        level = p.getInt("level", level);
        index = p.getInt("index", index);
        p.end();
        if (level < 0 || level >= LEVEL_COUNT) level = 1;
    }
    void save() const {
        Preferences p;
        p.begin("lightshow", false);
        p.putInt("pin", pin);
        p.putBool("zigzag", zigzag);
        p.putInt("rotate", rotate);
        p.putInt("level", level);
        p.putInt("index", index);
        p.end();
    }
};

String pinLabel(int pin) { return pin < 0 ? String("screen only") : "GPIO " + String(pin); }

// Minimal WS2812B driver on the ESP32 RMT peripheral, same timings as the core's
// rgbLedWrite() (10 MHz tick: 1 = 0.8/0.4 us, 0 = 0.4/0.8 us).
class Panel {
    int _pin = -1;
    rmt_data_t _sym[N * N * 24];

public:
    bool begin(int pin) {
        if (pin < 0) return false;
        if (!rmtInit(pin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_2, 10000000)) return false;
        _pin = pin;
        return true;
    }
    void show(const uint8_t *grb, int bytes) {
        if (_pin < 0) return;
        int s = 0;
        for (int i = 0; i < bytes; i++)
            for (int bit = 7; bit >= 0; bit--) {
                bool one = (grb[i] >> bit) & 1;
                _sym[s].level0 = 1;
                _sym[s].duration0 = one ? 8 : 4;
                _sym[s].level1 = 0;
                _sym[s].duration1 = one ? 4 : 8;
                s++;
            }
        rmtWrite(_pin, _sym, s, 100);
    }
    void end() {
        if (_pin < 0) return;
        uint8_t off[N * N * 3] = {};
        for (int k = 0; k < 2; k++) { // twice, so a frame cut short can't leave LEDs lit
            show(off, sizeof(off));
            delay(2);
        }
        rmtDeinit(_pin);
        pinMode(_pin, OUTPUT); // held low: a floating line can latch the last frame
        digitalWrite(_pin, LOW);
        _pin = -1;
    }
};

Panel panel; // ~6 KB of RMT symbols; static so it isn't on the task stack

int panelIndex(const Settings &s, int x, int y) {
    for (int r = 0; r < s.rotate / 90; r++) {
        int nx = N - 1 - y;
        y = x;
        x = nx;
    }
    if (s.zigzag && (y % 2)) x = N - 1 - x;
    return y * N + x;
}

uint16_t to565(RGB c) { return ((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3); }

void buildGamma(float g, uint8_t *table) {
    for (int i = 0; i < 256; i++) table[i] = (uint8_t)(powf(i / 255.0f, g) * 255 + 0.5f);
}

// Screen layout
struct Layout {
    int cell, gx, gy, px;
};
Layout layout() {
    Layout l;
    l.cell = min(20, (tftHeight - 58) / N);
    l.gx = 16;
    l.gy = 50;
    l.px = l.gx + N * l.cell + 14;
    return l;
}

void drawStatic(const Layout &l, const Settings &s) {
    drawMainBorderWithTitle("LIGHT SHOW");
    cyberRainbowFrame(l.gx - 3, l.gy - 3, N * l.cell + 5, N * l.cell + 5, 0);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    int y = l.gy + 92;
    tft.drawString("NEXT/PREV  scene", l.px, y, 1);
    tft.drawString("SEL        bright", l.px, y + 12, 1);
    tft.drawString("ESC        exit", l.px, y + 24, 1);
    tft.setTextColor(cyberHue(180), bruceConfig.bgColor);
    tft.drawString(pinLabel(s.pin), l.px, y + 42, 1);
}

void drawSceneInfo(const Layout &l, int index, int count, const char *label) {
    Scene *sc = scene(index);
    tft.fillRect(l.px, l.gy, tftWidth - l.px - 8, 60, bruceConfig.bgColor);
    String name = sc->name();
    tft.setTextSize(name.length() * 12 <= tftWidth - l.px - 10 ? 2 : 1);
    cyberRainbowText(name, l.px, l.gy, index * 20, bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawString(String(index + 1) + " of " + String(count), l.px, l.gy + 22, 1);
    if (label) {
        tft.setTextColor(cyberHue(index * 20 + 120), bruceConfig.bgColor);
        tft.drawString(label, l.px, l.gy + 36, 1);
    }
}

void drawLevel(const Layout &l, int level) {
    int y = l.gy + 80;
    for (int n = 0; n < LEVEL_COUNT; n++) {
        int h = (n + 1) * 2;
        tft.fillRect(l.px + n * 14, y - 10, 11, 10, bruceConfig.bgColor);
        tft.fillRect(l.px + n * 14, y - h, 11, h, n <= level ? cyberHue(40 + n * 30) : 0x2104);
    }
}

} // namespace

void LightShowMenu::optionsMenu() {
    Settings s;
    s.load();
    options = {
        {"Start show",                                  [this]() { run(); }    },
        {("Data pin: " + pinLabel(s.pin)).c_str(),      [this]() { pinMenu(); }},
        {(String("Zigzag rows: ") + (s.zigzag ? "on" : "off")).c_str(),
         [s]() mutable {
             s.zigzag = !s.zigzag;
             s.save();
         }                                                                     },
        {("Rotate: " + String(s.rotate)).c_str(),
         [s]() mutable {
             s.rotate = (s.rotate + 90) % 360;
             s.save();
         }                                                                     },
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Light Show");
}

void LightShowMenu::pinMenu() {
    Settings s;
    s.load();
    options = {};
    for (int i = 0; i < PIN_CHOICE_COUNT; i++) {
        int pin = PIN_CHOICES[i];
        String label = pinLabel(pin);
        if (pin == 22 || pin == 27) label += " (hat DIP off)";
        options.push_back({label.c_str(), [pin]() {
                               Settings t;
                               t.load();
                               t.pin = pin;
                               t.save();
                           },
                           pin == s.pin});
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Data pin");
}

void LightShowMenu::run() {
    Settings s;
    s.load();

    // Pins 22/27 are shared with the hat: refuse to fight a powered module.
    if (s.pin == 22 || s.pin == 27) {
        HatProbeResult hp = probeHat();
        if (hp.pn532 || hp.cc1101 || hp.nrf24) {
            displayWarning(hp.summary() + " is on. Turn DIP 1-5 OFF for the LED panel.", true);
            return;
        }
    }
    bool hasPanel = panel.begin(s.pin);

    const int count = sceneCount();
    int index = ((s.index % count) + count) % count;
    int level = s.level;
    int ORDER[N * N];
    for (int i = 0; i < N * N; i++) ORDER[i] = panelIndex(s, i % N, i / N);

    Layout l = layout();
    drawStatic(l, s);
    if (!hasPanel && s.pin >= 0) {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawString("LED pin init failed", l.px, l.gy + 150, 1);
    }

    RGB buf[N * N];
    uint8_t out[N * N * 3];
    uint16_t shown[N * N];
    for (int i = 0; i < N * N; i++) shown[i] = 0x0001; // force first paint
    uint8_t gam[256];
    buildGamma(scene(index)->gamma(), gam);
    const char *lastLabel = nullptr;
    drawSceneInfo(l, index, count, nullptr);
    drawLevel(l, level);

    float t = 0;
    uint32_t lastTick = millis(), lastPreview = 0;
    while (true) {
        uint32_t now = millis();
        t += (now - lastTick) / 1000.0f;
        lastTick = now;

        Scene *sc = scene(index);
        sc->frame(t, buf);

        if (hasPanel) {
            float k = LEVELS[level];
            for (int i = 0; i < N * N; i++) {
                int j = ORDER[i] * 3;
                out[j] = (uint8_t)(gam[buf[i].g] * k); // WS2812B wants green, red, blue
                out[j + 1] = (uint8_t)(gam[buf[i].r] * k);
                out[j + 2] = (uint8_t)(gam[buf[i].b] * k);
            }
            panel.show(out, sizeof(out));
        }

        if (now - lastPreview > 80) { // the screen doesn't need every frame
            lastPreview = now;
            for (int i = 0; i < N * N; i++) {
                uint16_t c = to565(buf[i]);
                if (c == shown[i]) continue;
                shown[i] = c;
                tft.fillRect(l.gx + (i % N) * l.cell, l.gy + (i / N) * l.cell, l.cell - 1, l.cell - 1, c);
            }
            const char *lab = sc->label();
            if (lab != lastLabel) {
                lastLabel = lab;
                drawSceneInfo(l, index, count, lab);
            }
        }

        bool changed = false;
        if (check(NextPress)) index = (index + 1) % count, changed = true;
        if (check(PrevPress)) index = (index + count - 1) % count, changed = true;
        if (check(SelPress)) {
            level = (level + 1) % LEVEL_COUNT;
            drawLevel(l, level);
            s.level = level;
            s.save();
        }
        if (check(EscPress)) break;
        if (changed) {
            s.index = index;
            s.save();
            buildGamma(scene(index)->gamma(), gam);
            lastLabel = nullptr;
            drawSceneInfo(l, index, count, scene(index)->label());
        }
        delay(15);
    }

    panel.end();
    returnToMenu = true;
}

void LightShowMenu::drawIcon(float scale) {
    clearIconArea();
    // A tiny 4x4 panel in rainbow.
    int cell = scale * 12, gap = scale * 3;
    int w = 4 * cell + 3 * gap;
    int x0 = iconCenterX - w / 2, y0 = iconCenterY - w / 2;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            tft.fillRoundRect(
                x0 + x * (cell + gap), y0 + y * (cell + gap), cell, cell, scale * 2, cyberHue((x + y) * 45)
            );
}
