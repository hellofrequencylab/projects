#include "SensorsMenu.h"

#include "../ui/cyber_icon.h"

#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/scrollableTextArea.h"
#include "core/utils.h"
#include "sensor_hub.h"
#include "sensor_packet.h"
#include <globals.h>
#include <math.h>

using namespace sensors;

namespace {

constexpr int ROW_H = 20;
constexpr int LIST_Y = 48;
constexpr uint16_t DIM = 0x8410;
constexpr float FEET_PER_M = 3.28084f;

uint16_t bg() { return bruceConfig.bgColor; }
int listRows() { return max(1, (tftHeight - 14 - LIST_Y) / ROW_H); }
uint16_t chanHue(int i) { return cyberHue(300 + i * 47); }
bool stale(const Channel &c, uint32_t now) { return c.wireless && now - c.updatedMs > WIRELESS_STALE_MS; }

String shortAgo(uint32_t ms) {
    uint32_t s = ms / 1000;
    if (s < 100) return String(s) + "s";
    if (s < 6000) return String(s / 60) + "m";
    return String(s / 3600) + "h";
}

float relAltitude(float p, float p0) { return 44330.0f * (1.0f - powf(p / p0, 0.190295f)); }

// Rainbow sparkline in a rainbow frame. Writes the min/max it scaled to.
void drawSpark(int x, int y, int w, int h, const float *v, int n, int phase, float &mn, float &mx) {
    tft.fillRect(x + 1, y + 1, w - 2, h - 2, bg());
    cyberRainbowFrame(x, y, w, h, phase);
    mn = mx = n ? v[0] : 0;
    if (n < 2) {
        tft.setTextSize(1);
        tft.setTextColor(DIM, bg());
        tft.drawCentreString("collecting...", x + w / 2, y + h / 2 - 4, 1);
        return;
    }
    for (int i = 1; i < n; i++) mn = min(mn, v[i]), mx = max(mx, v[i]);
    float span = mx - mn;
    int px = 0, py = 0;
    for (int i = 0; i < n; i++) {
        int X = x + 3 + i * (w - 7) / (n - 1);
        int Y = span < 1e-6f ? y + h / 2 : y + h - 4 - (int)((v[i] - mn) / span * (h - 8));
        if (i) {
            uint16_t col = cyberHue(phase + i * 300 / n);
            tft.drawLine(px, py, X, Y, col);
            tft.drawLine(px, py + 1, X, Y + 1, col);
        }
        px = X, py = Y;
    }
}

// Big shimmering number, unit to its right, centred.
void drawBigValue(const String &val, const char *unit, int y, int size, int phase) {
    tft.fillRect(8, y, tftWidth - 16, size * 8 + 2, bg());
    tft.setTextSize(size);
    int w = val.length() * size * LW;
    int uw = strlen(unit) * 2 * LW;
    int x = max(10, (tftWidth - w - uw - 6) / 2);
    cyberRainbowText(val, x, y, phase, bg());
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, bg());
    tft.drawString(unit, x + w + 6, y + size * 8 - 16, 1);
}

void drawFooter(const String &s, uint16_t color) {
    int y = tftHeight - 11;
    tft.fillRect(8, y, tftWidth - 16, 9, bg());
    tft.setTextSize(1);
    tft.setTextColor(color, bg());
    tft.drawCentreString(s, tftWidth / 2, y, 1);
}

// ---- dashboard: list ------------------------------------------------------------

void drawRow(int row, int idx, bool selected, uint32_t now) {
    int y = LIST_Y + row * ROW_H, x = 8, w = tftWidth - 16;
    tft.fillRect(x, y, w, ROW_H - 1, bg());
    Channel c;
    if (!channel(idx, c)) return;
    bool old = stale(c, now);
    uint16_t hue = old ? DIM : chanHue(idx);
    if (selected) cyberRainbowFrame(x, y, w, ROW_H - 1, cyberPhase());
    tft.fillRect(x + 3, y + 3, 3, ROW_H - 7, hue);
    tft.setTextSize(1);
    tft.setTextColor(hue, bg());
    tft.drawString(c.label, x + 10, y + 2, 1);
    tft.setTextColor(DIM, bg());
    tft.drawString(String(c.source) + (c.wireless ? " ))" : ""), x + 10, y + 11, 1);
    tft.setTextSize(2);
    tft.setTextColor(old ? DIM : TFT_WHITE, bg());
    tft.drawRightString(c.updatedMs ? formatValue(c.value, c.unit) : String("--"), tftWidth - 46, y + 2, 1);
    tft.setTextSize(1);
    tft.setTextColor(hue, bg());
    tft.drawString(unitText(c.unit), tftWidth - 42, y + 2, 1);
    if (c.wireless) {
        tft.setTextColor(old ? TFT_RED : DIM, bg());
        tft.drawString(shortAgo(now - c.updatedMs), tftWidth - 42, y + 11, 1);
    }
}

String statusText(const SessionInfo &si) {
    int wiredN = 0;
    for (const auto &d : wiredDevices())
        if (readable(d.chip)) wiredN++;
    String s = si.wiredOk ? String(wiredN) + " wired" : String("wired OFF");
    s += si.wirelessOk ? "  ESP-NOW ch " + String(si.espnowChannel) : String("  ESP-NOW off");
    if (loggingEnabled()) s += "  * REC";
    return s;
}

void drawEmpty(const SessionInfo &si) {
    tft.setTextSize(1);
    int y = LIST_Y + 6;
    cyberRainbowText("No sensors yet", 20, y, cyberPhase(), bg());
    tft.setTextColor(TFT_WHITE, bg());
    tft.drawString("Wired: I2C on the Grove port", 20, y + 20, 1);
    tft.drawString("  (SDA 27, SCL 22, 3V3, GND)", 20, y + 32, 1);
    tft.drawString("Wireless: ESP-NOW nodes on ch " + String(si.espnowChannel), 20, y + 50, 1);
    tft.setTextColor(DIM, bg());
    tft.drawString("Plug in, then reopen to rescan.", 20, y + 68, 1);
}

// ---- dashboard: one channel, big ------------------------------------------------

struct BigState {
    uint32_t shown = 0;
};

void drawBig(int idx, bool full, BigState &st, uint32_t now) {
    Channel c;
    if (!channel(idx, c)) return;
    int phase = cyberPhase();
    if (full) {
        drawMainBorderWithTitle(c.label);
        tft.setTextSize(1);
        tft.setTextColor(chanHue(idx), bg());
        String src = String(c.source) + (c.wireless ? "  via ESP-NOW" : "  via I2C");
        tft.drawCentreString(src, tftWidth / 2, LIST_Y, 1);
        st.shown = 0;
    }
    if (c.updatedMs != st.shown || full) {
        st.shown = c.updatedMs;
        drawBigValue(c.updatedMs ? formatValue(c.value, c.unit) : String("--"), unitText(c.unit), LIST_Y + 14, 4, phase);
        float v[HISTORY];
        for (int i = 0; i < c.histCount; i++) v[i] = c.histAt(i);
        float mn, mx;
        int sy = LIST_Y + 52, sh = tftHeight - 16 - sy;
        drawSpark(12, sy, tftWidth - 24, sh, v, c.histCount, phase, mn, mx);
        String f = "min " + formatValue(mn, c.unit) + "  max " + formatValue(mx, c.unit);
        if (c.wireless) {
            f += "  rssi " + String(c.rssi);
            if (c.battery != CU_SENSOR_BATTERY_NONE) f += "  batt " + String(c.battery) + "%";
            if (c.lost) f += "  lost " + String(c.lost);
        }
        drawFooter(f, cyberHue(phase + 180));
    }
}

void runDashboard() {
    displayInfo("Scanning sensors...");
    SessionInfo si = beginSession(true);
    if (!si.warning.isEmpty()) displayWarning(si.warning, true);
    if (!si.wirelessOk) displayWarning("ESP-NOW didn't start; wired only.", true);

    constexpr int MAX_ROWS = 12;
    uint32_t shownUpd[MAX_ROWS] = {};
    int shownIdx[MAX_ROWS] = {};
    bool shownSel[MAX_ROWS] = {};
    int sel = 0, top = 0, lastCount = -1;
    bool big = false, full = true;
    BigState bs;
    uint32_t lastTick = 0, lastAges = 0;
    while (!returnToMenu) {
        poll();
        uint32_t now = millis();
        int n = channelCount();
        if (n != lastCount) {
            lastCount = n;
            full = true;
            if (sel >= n) sel = max(0, n - 1);
        }

        if (big && n > 0) {
            if (full || now - lastTick > 150) {
                drawBig(sel, full, bs, now);
                full = false;
                lastTick = now;
            }
        } else if (full || now - lastTick > 300) {
            int rows = listRows();
            if (sel < top) top = sel;
            if (sel >= top + rows) top = sel - rows + 1;
            if (full) {
                drawMainBorderWithTitle("SENSORS");
                if (n == 0) drawEmpty(si);
                drawFooter(statusText(si), cyberHue(180));
            }
            // Repaint a row only when its value, the selection or (once a second) its age changed.
            bool ages = now - lastAges > 1000;
            for (int r = 0; n > 0 && r < rows && r < MAX_ROWS; r++) {
                int idx = top + r;
                Channel c;
                bool have = channel(idx, c);
                uint32_t upd = have ? c.updatedMs : 0;
                bool isSel = idx == sel;
                if (!full && upd == shownUpd[r] && idx == shownIdx[r] && isSel == shownSel[r] &&
                    !(ages && have && c.wireless))
                    continue;
                if (idx < n) drawRow(r, idx, isSel, now);
                else tft.fillRect(8, LIST_Y + r * ROW_H, tftWidth - 16, ROW_H - 1, bg());
                shownUpd[r] = upd, shownIdx[r] = idx, shownSel[r] = isSel;
            }
            if (ages) lastAges = now;
            full = false;
            lastTick = now;
        }

        if (check(EscPress)) {
            if (big) big = false, full = true;
            else break;
        }
        if (n > 0) {
            if (check(NextPress)) sel = (sel + 1) % n, full = full || big;
            if (check(PrevPress)) sel = (sel + n - 1) % n, full = full || big;
            if (check(SelPress)) big = !big, full = true;
        } else if (check(SelPress)) break;
        delay(10);
    }
    endSession();
}

// ---- altimeter ------------------------------------------------------------------

int findPressure() {
    int best = -1;
    Channel c;
    for (int i = 0; i < channelCount(); i++) {
        if (!channel(i, c) || c.type != CU_TYPE_PRESSURE || !c.updatedMs) continue;
        if (!c.wireless) return i; // wired first: it's on this device
        if (best < 0) best = i;
    }
    return best;
}

void runAltimeter() {
    displayInfo("Looking for a pressure sensor...");
    SessionInfo si = beginSession(true);
    if (!si.warning.isEmpty() && !si.wiredOk) displayWarning(si.warning, true);

    float p = 0, hist[HISTORY];
    int hn = 0;
    bool feet = false, full = true, haveSensor = false;
    uint32_t lastUpd = 0;
    String srcName;
    while (!returnToMenu) {
        poll();
        int idx = findPressure();
        if ((idx >= 0) != haveSensor) haveSensor = idx >= 0, full = true;

        if (!haveSensor) {
            if (full) {
                drawMainBorderWithTitle("ALTIMETER");
                tft.setTextSize(1);
                tft.setTextColor(TFT_WHITE, bg());
                tft.drawString("No pressure reading yet.", 20, LIST_Y + 10, 1);
                tft.drawString("Wired: BME280 / BMP280 on Grove.", 20, LIST_Y + 28, 1);
                tft.drawString("Wireless: a node sending type 3", 20, LIST_Y + 40, 1);
                tft.drawString("  (pressure, hPa) on ch " + String(si.espnowChannel) + ".", 20, LIST_Y + 52, 1);
                drawFooter("ESC back", DIM);
                full = false;
            }
        } else {
            Channel c;
            channel(idx, c);
            bool fresh = c.updatedMs != lastUpd;
            if (fresh) {
                lastUpd = c.updatedMs;
                p = p > 0 ? p * 0.8f + c.value * 0.2f : c.value; // a little smoothing on top of the chip's IIR
                if (altimeterBaseline() <= 0) setAltimeterBaseline(p);
                float alt = relAltitude(p, altimeterBaseline());
                if (hn < HISTORY) hist[hn++] = alt;
                else memmove(hist, hist + 1, (HISTORY - 1) * sizeof(float)), hist[HISTORY - 1] = alt;
            }
            if (full) {
                drawMainBorderWithTitle("ALTIMETER");
                srcName = String(c.source) + (c.wireless ? " (ESP-NOW)" : "");
            }
            if (fresh || full) {
                int phase = cyberPhase();
                float p0 = altimeterBaseline();
                float alt = relAltitude(p, p0);
                float shown = feet ? alt * FEET_PER_M : alt;
                String v = (shown >= 0 ? "+" : "") + String(shown, fabsf(shown) < 100 ? 1 : 0);
                drawBigValue(v, feet ? "ft" : "m", LIST_Y, 5, phase);
                tft.setTextSize(1);
                tft.fillRect(8, LIST_Y + 44, tftWidth - 16, 20, bg());
                tft.setTextColor(cyberHue(phase + 120), bg());
                float absM = relAltitude(p, 1013.25f);
                tft.drawCentreString(
                    String(p, 1) + " hPa   zero " + String(p0, 1) + "   std " + String(absM, 0) + " m",
                    tftWidth / 2,
                    LIST_Y + 44,
                    1
                );
                tft.setTextColor(DIM, bg());
                tft.drawCentreString(srcName, tftWidth / 2, LIST_Y + 55, 1);
                float graph[HISTORY], mn, mx;
                for (int i = 0; i < hn; i++) graph[i] = feet ? hist[i] * FEET_PER_M : hist[i];
                int sy = LIST_Y + 68;
                drawSpark(12, sy, tftWidth - 24, tftHeight - 16 - sy, graph, hn, phase, mn, mx);
                drawFooter("SEL zero   NEXT m/ft   ESC back", cyberHue(phase + 200));
                full = false;
            }
        }

        if (check(EscPress)) break;
        if (haveSensor && check(SelPress)) {
            setAltimeterBaseline(p);
            hn = 0;
            full = true;
        }
        if (check(NextPress) || check(PrevPress)) feet = !feet, full = true;
        delay(10);
    }
    endSession();
}

// ---- other screens ----------------------------------------------------------------

void runScan() {
    displayInfo("Scanning I2C...");
    ScrollableTextArea area("I2C BUS");
    for (const String &l : scanReport()) area.addLine(l);
    area.show();
}

void showProtocol() {
    ScrollableTextArea area("NODE PROTOCOL");
    area.addLine("ESP-NOW broadcast or unicast,");
    area.addLine("Wi-Fi channel " + String(CU_SENSOR_CHANNEL) + " (or the hub's AP");
    area.addLine("channel if it is on Wi-Fi).");
    area.addLine("");
    area.addLine("28-byte packet, little-endian:");
    area.addLine(" 0 magic 0x5543  2 version 1");
    area.addLine(" 3 type   4 node_id (u32)");
    area.addLine(" 8 seq (u16)  10 battery %");
    area.addLine("11 unit  12 value (float)");
    area.addLine("16 name (12 chars)");
    area.addLine("");
    area.addLine("One reading per packet.");
    area.addLine("Header: apps/sensors/");
    area.addLine("  sensor_packet.h");
    area.addLine("Docs: apps/sensors/PROTOCOL.md");
    area.show();
}

} // namespace

void SensorsMenu::optionsMenu() {
    while (!returnToMenu) {
        options = {
            {"Live sensors", []() { runDashboard(); }},
            {"Altimeter", []() { runAltimeter(); }},
            {"Scan I2C bus", []() { runScan(); }},
            {(String("Logging: ") + (loggingEnabled() ? "on" : "off")).c_str(),
             []() {
                 setLoggingEnabled(!loggingEnabled());
                 if (loggingEnabled()) displayInfo("Every 10 s to " + logTarget(), true);
             }},
            {"Node protocol", []() { showProtocol(); }},
        };
        addOptionToMainMenu();
        int picked = loopOptions(options, MENU_TYPE_SUBMENU, "Sensors");
        if (picked < 0 || picked == (int)options.size() - 1) break;
    }
}

void SensorsMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

// ---- CLI --------------------------------------------------------------------------

namespace {

uint32_t sensorsCallback(cmd *c) {
    Command cmd(c);
    String action = cmd.getArgument("action").getValue();
    String arg = cmd.getArgument("value").getValue();
    action.trim();
    arg.trim();

    if (action == "scan") {
        for (const String &l : scanReport()) serialDevice->println(l);
        return true;
    }
    if (action == "log") {
        if (arg == "on" || arg == "off") setLoggingEnabled(arg == "on");
        serialDevice->println(String("Logging ") + (loggingEnabled() ? "on -> " + logTarget() : String("off")));
        return true;
    }
    if (action == "listen") {
        int secs = arg.toInt();
        if (secs <= 0) secs = 10;
        secs = min(secs, 120);
        SessionInfo si = beginSession(true);
        if (!si.wirelessOk) serialDevice->println("ESP-NOW failed to start.");
        else serialDevice->printf("Listening on ESP-NOW channel %d for %d s...\n", si.espnowChannel, secs);
        uint32_t end = millis() + secs * 1000UL;
        while ((int32_t)(millis() - end) < 0) {
            poll();
            delay(20);
        }
        for (const String &l : readingsReport()) serialDevice->println(l);
        endSession();
        return true;
    }
    if (action != "" && action != "read") {
        serialDevice->println("usage: sensors [read|scan|listen [seconds]|log [on|off]]");
        return false;
    }

    // Read: if the Sensors screen is open, report what it has; otherwise do a short
    // wired-only session of our own (long enough for every driver's first reading).
    bool own = !sessionActive();
    if (own) {
        beginSession(false);
        uint32_t end = millis() + 1300;
        while ((int32_t)(millis() - end) < 0) {
            poll();
            delay(10);
        }
    }
    for (const String &l : readingsReport()) serialDevice->println(l);
    if (own) endSession();
    return true;
}

} // namespace

void createSensorsCommand(SimpleCLI *cli) {
    Command c = cli->addCommand("sensors", sensorsCallback);
    c.addPosArg("action", "read");
    c.addPosArg("value", "");
}
