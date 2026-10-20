#include "WeatherMenu.h"

#include "../ui/cyber_icon.h"

#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "weather.h"
#include <WiFi.h>
#include <globals.h>
#include <math.h>

using namespace weather;

namespace {

// ---------- pixel-art condition icons: 12x12, '.' is see-through ----------

const char *const SUN[12] = {
    ".....YY.....", "..Y..YY..Y..", "...Y....Y...", "....OOOO....", "...OYYYYO...", "YY.OYYYYO.YY",
    "YY.OYYYYO.YY", "...OYYYYO...", "....OOOO....", "...Y....Y...", "..Y..YY..Y..", ".....YY.....",
};
const char *const MOON[12] = {
    "....LLL.....", "...LLW...*..", "..LLW.......", "..LLW.......", ".LLLW.......", ".LLLW....*..",
    ".LLLLW......", ".LLLLLW.....", "..LLLLLWWWL.", "..LLLLLLLLL.", "...LLLLLLL..", ".....LLL....",
};
const char *const CLOUD[12] = {
    "............", "............", "....WWW.....", "...WWWWW.WW.", "..WWWWWWWWWW", ".WWWWWWWWWWW",
    "WWWWWWWWWWWW", "WWWWWWWWWWWW", ".MMMMMMMMMM.", "............", "............", "............",
};
const char *const FOG[12] = {
    "............", "............", "GGGGGGGG....", "............", "..GGGGGGGGGG", "............",
    "GGGGGGGGGG..", "............", "...GGGGGGGGG", "............", "GGGGGGG.....", "............",
};
// Precipitation rows, drawn under the cloud (rows 9-11).
const char *const DRIZZLE[3] = {"..B.....B...", "............", ".....B......"};
const char *const RAIN[3] = {"..B..B..B...", ".B..B..B..B.", "B..B..B..B.."};
const char *const SNOW[3] = {".S...S...S..", "...S...S...S", ".S...S...S.."};
const char *const BOLT[3] = {".....YY.....", "....YY......", ".....Y......"};

uint16_t pixelColor(char c) {
    switch (c) {
        case 'Y': return cyberHue(52);
        case 'O': return cyberHue(28);
        case 'W': return TFT_WHITE;
        case 'M': return cyberHue(300); // magenta cloud belly: synthwave
        case 'G': return 0xA534;
        case 'B': return cyberHue(190);
        case 'S': return 0xDF1F;
        case 'L': return 0xCE7F; // lilac moon
        case '*': return cyberHue(180);
        default: return 0;
    }
}

void sprite(const char *const *rows, int nrows, int x, int y, int s) {
    for (int r = 0; r < nrows; r++)
        for (int c = 0; c < 12 && rows[r][c]; c++)
            if (rows[r][c] != '.') tft.fillRect(x + c * s, y + r * s, s, s, pixelColor(rows[r][c]));
}

void drawCondition(int code, bool day, int x, int y, int s) {
    const char *const *sky = day ? SUN : MOON;
    if (code <= 1) return sprite(sky, 12, x, y, s);
    if (code == 45 || code == 48) return sprite(FOG, 12, x, y, s);
    if (code == 2) { // partly cloudy: sun/moon peeking out behind a cloud
        sprite(sky, 12, x - 2 * s, y - 2 * s, s);
        return sprite(CLOUD, 12, x + s, y + 2 * s, s);
    }
    sprite(CLOUD, 12, x, y, s);
    const char *const *under = nullptr;
    if (code >= 51 && code <= 57) under = DRIZZLE;
    else if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) under = RAIN;
    else if ((code >= 71 && code <= 77) || code == 85 || code == 86) under = SNOW;
    else if (code >= 95) under = BOLT;
    if (under) sprite(under, 3, x, y + 9 * s, s);
}

// ---------- synthwave backdrop ----------

int horizonY() { return tftHeight * 61 / 100; }

uint16_t mix565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }

void drawBackdrop(bool sun) {
    const int hy = horizonY(), W = tftWidth, H = tftHeight;
    // Sky: midnight blue fading to hot purple at the horizon.
    for (int y = 0; y < hy; y++) {
        int k = y * 255 / hy;
        tft.drawFastHLine(0, y, W, mix565(10 + k * 90 / 255, 0 + k * 10 / 255, 40 + k * 60 / 255));
    }
    // Striped synthwave sun sinking into the horizon, bottom right.
    if (sun) {
        const int cx = W - 52, r = 30;
        for (int dy = -r; dy < 0; dy++) {
            int band = -dy; // gaps get wider toward the horizon
            if (band < 14 && (band % 5) < (5 - band / 3)) continue;
            int half = (int)sqrtf((float)(r * r - dy * dy));
            int k = (dy + r) * 255 / r;
            tft.drawFastHLine(cx - half, hy + dy, 2 * half, mix565(255, 220 - k * 180 / 255, 60 + k * 120 / 255));
        }
    }
    // Floor: black with a magenta perspective grid.
    tft.fillRect(0, hy, W, H - hy, TFT_BLACK);
    const uint16_t grid = cyberHue(300);
    cyberRainbowHLine(0, hy, W, 0);
    for (int i = 1, y = hy + 3; y < H; i++, y += 2 + i * 2) tft.drawFastHLine(0, y, W, grid);
    for (int i = -8; i <= 8; i++) tft.drawLine(W / 2 + i * 12, hy, W / 2 + i * 60, H, grid);
}

// ---------- text helpers (transparent background over the gradient) ----------

void text(const String &s, int x, int y, int size, uint16_t color) {
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.drawString(s, x, y, 1);
}

// A temperature in its rainbow colour with a drawn degree ring (the font has none).
// Returns the x just past it.
int temp(float t, bool metric, int x, int y, int size, bool unit) {
    String s = String((int)lroundf(t));
    uint16_t c = cyberHue(tempHue(t, metric));
    text(s, x, y, size, c);
    x += s.length() * 6 * size;
    int r = max(1, size);
    tft.drawCircle(x + r + 1, y + r + 1, r, c);
    if (size > 2) tft.drawCircle(x + r + 1, y + r + 1, r - 1, c);
    x += 2 * r + 3;
    if (unit) {
        text(metric ? "C" : "F", x, y, max(1, size / 2), c);
        x += 6 * max(1, size / 2);
    }
    return x;
}

const char *DAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

void drawOutdoor(const Data &d) {
    const int W = tftWidth;
    drawBackdrop(true);
    tft.setTextSize(2);
    String city = d.city.length() > 16 ? d.city.substring(0, 16) : d.city;
    cyberRainbowText(city, 8, 6, cyberPhase(), mix565(10, 0, 40));
    text(d.localTime, W - 6 - d.localTime.length() * 6, 6, 1, cyberHue(180));
    text("SEL refresh", W - 72, 16, 1, 0x8C71);
    cyberRainbowHLine(8, 26, W - 16, cyberPhase());

    int x = temp(d.temp, d.metric, 10, 36, 6, true);
    drawCondition(d.code, d.isDay, W - 84, 32, 6);
    (void)x;

    text(describe(d.code), 10, 90, 2, cyberHue(180));
    int y = 112;
    text("FEELS", 10, y, 1, 0x8C71);
    int fx = temp(d.feels, d.metric, 46, y, 1, false);
    text("HI", fx + 10, y, 1, 0x8C71);
    fx = temp(d.hi[0], d.metric, fx + 26, y, 1, false);
    text("LO", fx + 8, y, 1, 0x8C71);
    temp(d.lo[0], d.metric, fx + 24, y, 1, false);
    y += 12;
    String hum = "HUMID " + String((int)lroundf(d.humidity)) + "%";
    text(hum, 10, y, 1, cyberHue(200));
    String wind = "WIND " + String((int)lroundf(d.wind)) + (d.metric ? " km/h" : " mph");
    text(wind, 10 + (hum.length() + 2) * 6, y, 1, cyberHue(140));
    y += 12;
    String press = d.metric ? String((int)lroundf(d.pressure)) + " hPa" : String(d.pressure, 2) + " inHg";
    text("PRESSURE " + press, 10, y, 1, cyberHue(260));

    // 3-day strip on the grid floor.
    const int hy = horizonY(), gap = 6, pw = (W - 4 * gap) / DAYS, ph = tftHeight - hy - 8;
    for (int i = 0; i < DAYS; i++) {
        int px = gap + i * (pw + gap), py = hy + 4;
        tft.fillRoundRect(px, py, pw, ph, 6, TFT_BLACK);
        cyberRainbowFrame(px, py, pw, ph, i * 120);
        text(i == 0 ? "TODAY" : DAY_NAMES[d.weekday[i] % 7], px + 6, py + 5, 1, TFT_WHITE);
        drawCondition(d.dayCode[i], true, px + 6, py + 18, 3);
        temp(d.hi[i], d.metric, px + pw - 40, py + 20, 2, false);
        temp(d.lo[i], d.metric, px + pw - 40, py + 42, 2, false);
    }
}

void drawIndoor(const Data &d, bool haveOutdoor) {
    drawBackdrop(false);
    const int W = tftWidth;
    tft.setTextSize(2);
    cyberRainbowText("INDOOR", 8, 6, cyberPhase(), mix565(10, 0, 40));
    text("NEXT outdoor", W - 78, 10, 1, 0x8C71);
    cyberRainbowHLine(8, 26, W - 16, cyberPhase());
    Settings s;
    s.load();
    float c, h;
    if (indoorClimate(c, h)) {
        float t = s.metric ? c : c * 9 / 5 + 32;
        temp(t, s.metric, 10, 40, 6, true);
        if (!isnan(h)) text("HUMID " + String((int)lroundf(h)) + "%", 10, 96, 2, cyberHue(200));
    } else {
        text("No indoor sensor yet", 10, 44, 2, cyberHue(300));
        text("Plug one into Grove and open the", 10, 70, 1, TFT_WHITE);
        text("Sensors app: it feeds this page.", 10, 82, 1, TFT_WHITE);
    }
    if (haveOutdoor) {
        text("OUTSIDE", 10, 120, 1, 0x8C71);
        temp(d.temp, d.metric, 58, 120, 1, true);
    }
}

void drawMessage(const String &title, const String &line1, const String &line2) {
    drawBackdrop(true);
    tft.setTextSize(2);
    cyberRainbowText(title, 10, 40, cyberPhase(), mix565(10, 0, 40));
    text(line1, 10, 72, 1, TFT_WHITE);
    text(line2, 10, 86, 1, cyberHue(180));
}

Data cache; // last good reading, kept while the device is up
const uint32_t STALE_MS = 10 * 60 * 1000UL;
const uint32_t AUTO_REFRESH_MS = 15 * 60 * 1000UL;

String locationLabel(const Settings &s) {
    if (!s.manual) return "auto (IP)";
    if (s.city.length()) return s.city;
    return String(s.lat, 2) + "," + String(s.lon, 2);
}

} // namespace

void WeatherMenu::optionsMenu() {
    Settings s;
    s.load();
    options = {
        {"Show weather", [this]() { run(); }},
        {(String("Units: ") + (s.metric ? "metric" : "imperial")).c_str(),
         [s]() mutable {
             s.metric = !s.metric;
             s.save();
             cache.ok = false; // numbers are in the old units
         }},
        {("Location: " + locationLabel(s)).c_str(), [this]() { locationMenu(); }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Weather");
}

void WeatherMenu::locationMenu() {
    Settings s;
    s.load();
    options = {
        {"Estimate from IP",
         [s]() mutable {
             s.manual = false;
             s.save();
             cache.ok = false;
         },
         !s.manual},
        {"Set lat,lon...",
         [s]() mutable {
             String cur = s.manual ? String(s.lat, 4) + "," + String(s.lon, 4) : String("");
             String in = keyboard(cur, 30, "lat,lon  e.g. 37.77,-122.42");
             if (in == "\x1B") return;
             int comma = in.indexOf(',');
             float lat = in.substring(0, comma).toFloat(), lon = in.substring(comma + 1).toFloat();
             if (comma < 0 || lat < -90 || lat > 90 || lon < -180 || lon > 180 || (lat == 0 && lon == 0)) {
                 displayError("Use lat,lon like 37.77,-122.42", true);
                 return;
             }
             String city = keyboard(s.city, 24, "City name (optional)");
             if (city == "\x1B") city = "";
             s.manual = true;
             s.lat = lat;
             s.lon = lon;
             s.city = city;
             s.save();
             cache.ok = false;
         },
         s.manual},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "Location");
}

void WeatherMenu::run() {
    bool indoor = false;
    bool needFetch = !cache.ok || millis() - cache.fetchedAt > STALE_MS;
    bool redraw = true;
    String error;
    uint32_t lastIndoor = 0;

    while (true) {
        if (needFetch) {
            needFetch = false;
            if (WiFi.status() != WL_CONNECTED) {
                error = "wifi";
            } else {
                drawMessage("Asking the sky...", "Open-Meteo, no key needed", "");
                Data d;
                if (fetchWeather(d)) {
                    cache = d;
                    error = "";
                } else {
                    error = d.error;
                }
            }
            redraw = true;
        }
        if (redraw) {
            redraw = false;
            if (indoor) drawIndoor(cache, cache.ok);
            else if (error == "wifi" && !cache.ok)
                drawMessage("Wi-Fi needed", "Outdoor weather comes from the internet.", "SEL connect   NEXT indoor   ESC back");
            else if (!cache.ok)
                drawMessage("No forecast", error.substring(0, 52), "SEL retry   NEXT indoor   ESC back");
            else drawOutdoor(cache);
            lastIndoor = millis();
        }

        if (check(EscPress)) break;
        if (check(NextPress) || check(PrevPress)) {
            indoor = !indoor;
            redraw = true;
        }
        if (check(SelPress)) {
            if (!indoor && WiFi.status() != WL_CONNECTED) {
                wifiConnectMenu(WIFI_MODE_STA); // the user's choice; we never join on our own
            }
            needFetch = !indoor;
            redraw = true;
        }
        float c, h;
        if (indoor && millis() - lastIndoor > 5000 && indoorClimate(c, h)) redraw = true; // sensor refresh
        if (!indoor && cache.ok && millis() - cache.fetchedAt > AUTO_REFRESH_MS &&
            WiFi.status() == WL_CONNECTED)
            needFetch = true;
        delay(30);
    }
    returnToMenu = true;
}

void WeatherMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}
