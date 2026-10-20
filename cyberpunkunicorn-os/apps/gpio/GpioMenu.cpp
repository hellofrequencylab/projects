#include "GpioMenu.h"

#include "../hat/hat_probe.h"
#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/utils.h"
#include <esp32-hal-periman.h>
#include <globals.h>

namespace {

struct FreePin {
    int pin;
    const char *where;
    bool inputOnly; // GPIO34-39: no output, no internal pull resistors
    bool hatShared; // every hat module uses 22/27
};

// Everything else on the CYD is taken: TFT 2/12/13/14/15/21, touch 25/32/33/36/39,
// SD + hat SPI 5/18/19/23, flash 6-11, USB serial 1/3, BOOT 0, RGB LED 4/16/17,
// LDR 34, speaker 26.
const FreePin PINS[] = {
    {27, "CN1 / Grove SDA, hat CS",   false, true },
    {22, "P3+CN1 / Grove SCL, hat CE", false, true },
    {35, "P3, input only",            true,  false},
};

enum Mode { M_READ, M_READ_UP, M_READ_DOWN, M_TOGGLE, M_PWM };

// 22/27 belong to whichever hat module is switched on: refuse to fight a
// module that answers on its bus. IR/433 can't be detected reliably, so those
// only get the warning.
bool hatAllows(const FreePin &p) {
    if (!p.hatShared) return true;
    displayInfo("Checking the hat...");
    HatProbeResult hp = probeHat();
    if (hp.pn532 || hp.cc1101 || hp.nrf24) {
        displayWarning(hp.summary() + " is on. Turn DIP 1-5 OFF to use GPIO" + String(p.pin) + ".", true);
        return false;
    }
    displayWarning("GPIO" + String(p.pin) + " is shared with the hat: keep DIP 1-5 OFF.", true);
    return true;
}

constexpr int X = 16, W = 288;
constexpr int TRACE_Y = 128, TRACE_H = 60;

void header(const FreePin &p, const char *mode, const char *hint) {
    drawMainBorderWithTitle("GPIO " + String(p.pin));
    tft.setTextSize(1);
    tft.setTextColor(cyberHue(180), bruceConfig.bgColor);
    tft.drawString(String(mode) + "  -  " + p.where, X, 44, 1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawString(hint, X, TRACE_Y + TRACE_H + 8, 1);
}

// Big HIGH/LOW lamp: a rainbow word when high, a dim one when low.
void drawLevel(bool high) {
    tft.fillRect(X, 58, 170, 34, bruceConfig.bgColor);
    tft.setTextSize(4);
    if (high) cyberRainbowText("HIGH", X, 60, cyberPhase(), bruceConfig.bgColor);
    else {
        tft.setTextColor(0x4208, bruceConfig.bgColor);
        tft.drawString("LOW", X, 60, 1);
    }
    tft.fillCircle(X + W - 20, 75, 14, high ? cyberHue(cyberPhase()) : 0x2104);
    tft.drawCircle(X + W - 20, 75, 16, cyberHue(300));
}

void readScreen(const FreePin &p, Mode m) {
    pinMode(p.pin, m == M_READ_UP ? INPUT_PULLUP : m == M_READ_DOWN ? INPUT_PULLDOWN : INPUT);
    const char *name = m == M_READ_UP ? "read, pull-up" : m == M_READ_DOWN ? "read, pull-down" : "read, floating";
    header(p, name, "SEL reset count    ESC exit");
    cyberRainbowFrame(X - 2, TRACE_Y - 2, W + 4, TRACE_H + 4, 0);
    bool adc = p.pin >= 32; // ADC1 works alongside Wi-Fi; ADC2 (22/27 are not ADC anyway) doesn't
    int x = 0, edges = 0, last = -1;
    uint32_t lastDraw = 0;
    while (true) {
        int v = digitalRead(p.pin);
        if (last >= 0 && v != last) edges++;
        if (v != last) drawLevel(v);
        last = v;
        if (millis() - lastDraw >= 40) { // scrolling logic trace, left to right
            lastDraw = millis();
            int y = v ? TRACE_Y + 6 : TRACE_Y + TRACE_H - 6;
            tft.drawFastVLine(X + x, TRACE_Y, TRACE_H, bruceConfig.bgColor);
            tft.drawFastVLine(X + (x + 1) % W, TRACE_Y, TRACE_H, TFT_WHITE); // cursor
            tft.fillRect(X + x, y - 1, 1, 3, v ? cyberHue(x * 2) : 0x52AA);
            x = (x + 1) % W;
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            String info = "edges " + String(edges);
            if (adc) info += "   analog " + String(analogReadMilliVolts(p.pin)) + " mV";
            tft.drawString(info + "      ", X, 100, 1);
        }
        if (check(SelPress)) edges = 0;
        if (check(EscPress)) break;
        delay(2);
    }
}

void toggleScreen(const FreePin &p) {
    header(p, "output", "SEL toggle    ESC exit (pin released)");
    pinMode(p.pin, OUTPUT);
    bool high = false;
    digitalWrite(p.pin, LOW);
    drawLevel(high);
    while (true) {
        if (check(SelPress)) {
            high = !high;
            digitalWrite(p.pin, high);
            drawLevel(high);
        }
        if (check(EscPress)) break;
        delay(10);
    }
}

void pwmScreen(const FreePin &p) {
    const uint32_t FREQS[] = {50, 1000, 5000};
    int fi = 1, duty = 50; // percent
    header(p, "PWM", "NEXT/PREV duty  SEL freq  ESC exit");
    cyberRainbowFrame(X - 2, TRACE_Y - 2, W + 4, TRACE_H + 4, 0);
    bool attached = false;
    bool redraw = true;
    while (true) {
        if (redraw) {
            redraw = false;
            // Re-attach on a frequency change instead of ledcChangeFrequency:
            // 5 kHz/8 bit shares the backlight's timer and must not be retuned.
            if (attached) ledcDetach(p.pin);
            attached = ledcAttach(p.pin, FREQS[fi], 8);
            if (attached) ledcWrite(p.pin, duty * 255 / 100);
            tft.fillRect(X, 58, W, 34, bruceConfig.bgColor);
            tft.setTextSize(3);
            cyberRainbowText(String(duty) + "%", X, 62, duty * 3, bruceConfig.bgColor);
            tft.setTextSize(2);
            tft.setTextColor(attached ? TFT_WHITE : TFT_RED, bruceConfig.bgColor);
            tft.drawString(attached ? String(FREQS[fi]) + " Hz" : String("LEDC busy"), X + 150, 66, 1);
            // two periods of the waveform
            tft.fillRect(X, TRACE_Y, W, TRACE_H, bruceConfig.bgColor);
            int hi = TRACE_Y + 8, lo = TRACE_Y + TRACE_H - 8, per = W / 2;
            for (int k = 0; k < 2; k++) {
                int x0 = X + k * per, xh = per * duty / 100;
                uint16_t c = cyberHue(300 + k * 60);
                if (xh) tft.drawFastHLine(x0, hi, xh, c);
                if (per - xh) tft.drawFastHLine(x0 + xh, lo, per - xh, c);
                tft.drawFastVLine(x0, hi, lo - hi, c);
                if (xh && xh < per) tft.drawFastVLine(x0 + xh, hi, lo - hi, c);
            }
        }
        if (check(NextPress)) duty = min(duty + 10, 100), redraw = true;
        if (check(PrevPress)) duty = max(duty - 10, 0), redraw = true;
        if (check(SelPress)) fi = (fi + 1) % 3, redraw = true;
        if (check(EscPress)) break;
        delay(10);
    }
    if (attached) ledcDetach(p.pin);
}

void pinTool(const FreePin &p) {
    if (!hatAllows(p)) return;
    std::vector<Option> modes = {
        {"Read (floating)", [&p]() { readScreen(p, M_READ); }},
    };
    if (!p.inputOnly) {
        modes.push_back({"Read (pull-up)", [&p]() { readScreen(p, M_READ_UP); }});
        modes.push_back({"Read (pull-down)", [&p]() { readScreen(p, M_READ_DOWN); }});
        modes.push_back({"Toggle output", [&p]() { toggleScreen(p); }});
        modes.push_back({"PWM", [&p]() { pwmScreen(p); }});
    }
    int idx = 0;
    while (true) {
        idx = loopOptions(modes, MENU_TYPE_SUBMENU, ("GPIO " + String(p.pin)).c_str(), idx);
        pinMode(p.pin, INPUT); // always hand the pin back floating
        if (idx < 0 || returnToMenu) break;
    }
}

} // namespace

void GpioMenu::optionsMenu() {
    int idx = 0;
    while (true) {
        options = {};
        for (const FreePin &p : PINS) {
            String label = "GPIO " + String(p.pin) + (p.hatShared ? "  (hat DIP off)" : "  (input only)");
            options.push_back({label.c_str(), [&p]() { pinTool(p); }});
        }
        addOptionToMainMenu();
        idx = loopOptions(options, MENU_TYPE_SUBMENU, "GPIO", idx);
        if (idx < 0 || returnToMenu) break;
    }
}

void GpioMenu::drawIcon(float scale) {
    clearIconArea();
    // A chip with rainbow legs.
    int w = scale * 34, h = scale * 44;
    int x = iconCenterX - w / 2, y = iconCenterY - h / 2;
    tft.fillRoundRect(x, y, w, h, scale * 3, 0x2104);
    tft.drawRoundRect(x, y, w, h, scale * 3, bruceConfig.priColor);
    tft.fillCircle(x + scale * 7, y + scale * 7, max(2, (int)(scale * 3)), bruceConfig.priColor);
    int legs = 5, leg = scale * 9, th = max(2, (int)(scale * 3));
    for (int i = 0; i < legs; i++) {
        int ly = y + scale * 5 + i * (h - scale * 10) / (legs - 1) - th / 2;
        tft.fillRect(x - leg, ly, leg, th, cyberHue(i * 36));
        tft.fillRect(x + w, ly, leg, th, cyberHue(180 + i * 36));
    }
}

static uint32_t pinsCallback(cmd *c) {
    for (const FreePin &p : PINS) {
        peripheral_bus_type_t t = perimanGetPinBusType(p.pin);
        char buf[128];
        snprintf(
            buf, sizeof(buf), "GPIO%-2d  level %d  owner %-14s  %s%s", p.pin, gpio_get_level((gpio_num_t)p.pin),
            perimanGetTypeName(t), p.where, p.hatShared ? " (DIP 1-5 OFF to use)" : ""
        );
        serialDevice->println(String(buf));
    }
    return true;
}

void createPinsCommand(SimpleCLI *cli) { cli->addCommand("pins", pinsCallback); }
