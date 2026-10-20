#include "cyber_icon.h"

#include "core/display.h"
#include "cyber_icons.h"
#include "rainbow.h"
#include "scene.h"
#include <globals.h>
#include <math.h>

namespace {

const CyberIcon *findIcon(const char *menu) {
    if (!menu) return nullptr;
    for (int i = 0; i < CYI_COUNT; i++)
        if (strcmp(cyi_icons[i].menu, menu) == 0) return &cyi_icons[i];
    return nullptr;
}

// The icon for a submenu title: the menu's own name, a few titles Bruce
// spells differently, or "<name> (...)" ("GPS (9600 bps)").
const CyberIcon *headerIcon(const char *title) {
    static const char *const ALIAS[][2] = {{"Bluetooth", "BLE"}, {"Infrared", "IR"}, {"Radio Frequency", "RF"}};
    if (!title) return nullptr;
    if (const CyberIcon *ic = findIcon(title)) return ic;
    for (const auto &a : ALIAS)
        if (strcmp(title, a[0]) == 0) return findIcon(a[1]);
    for (int i = 0; i < CYI_COUNT; i++) {
        size_t n = strlen(cyi_icons[i].menu);
        if (strncmp(title, cyi_icons[i].menu, n) == 0 && strncmp(title + n, " (", 2) == 0) return &cyi_icons[i];
    }
    return nullptr;
}

int iconHue(uint8_t mode, int x, int y) {
    switch (mode) {
        case CYI_ROW: return y * 18;
        case CYI_COL: return x * 12;
        case CYI_DIAG: return (x + y) * 10;
        default: return (int)(atan2f(y - (CYI_H - 1) / 2.0f, x - (CYI_W - 1) / 2.0f) * RAD_TO_DEG) + 180;
    }
}

bool animated(uint8_t idx) {
    return idx == CYI_RAINBOW || idx == CYI_SPARKLE || idx == CYI_FRAME_A || idx == CYI_FRAME_B ||
           idx == CYI_GRID_A || idx == CYI_GRID_B;
}

// Colour of an animated cell at tick t. Off frames paint the background, so a
// redraw needs no clear.
uint16_t animColor(const CyberIcon &ic, uint8_t idx, int x, int y, int t, uint16_t bg) {
    const int frame = (t / 6) & 1;
    switch (idx) {
        case CYI_RAINBOW: return cyberHue(iconHue(ic.mode, x, y) + t * 12);
        case CYI_FRAME_A: return frame ? bg : cyberHue(iconHue(ic.mode, x, y) + t * 12);
        case CYI_FRAME_B: return frame ? cyberHue(iconHue(ic.mode, x, y) + t * 12) : bg;
        case CYI_GRID_A: return frame ? cyi_palette[CYI_FLOOR] : cyi_palette[idx];
        case CYI_GRID_B: return frame ? cyi_palette[idx] : cyi_palette[CYI_FLOOR];
        default: { // sparkle: white, cyan, hot pink, off
            static const uint16_t tw[3] = {0xFFFF, 0x07FF, 0xFAD9};
            int p = (x * 3 + y * 5 + t / 4) % 6;
            return p > 2 ? bg : tw[p];
        }
    }
}

// Walks the run-length data. Static runs become one fillRect per row span;
// animated cells are drawn one by one. `animOnly` skips the static ones.
void drawIcon(const CyberIcon &ic, int cx, int cy, int cell, int t, bool animOnly) {
    const int x0 = cx - CYI_W * cell / 2, y0 = cy - CYI_H * cell / 2;
    const uint8_t *p = cyi_rle + ic.off;
    for (int pos = 0; pos < CYI_W * CYI_H;) {
        uint8_t b = *p++;
        uint8_t idx = b & 0x1F;
        int n = (b >> 5) + 1;
        if (n == 8) n = *p++ + 8;
        if (idx == CYI_TRANSPARENT) {
            pos += n;
        } else if (animated(idx)) {
            for (; n > 0; n--, pos++) {
                int x = pos % CYI_W, y = pos / CYI_W;
                // Off cells take the colour behind them: black, or the sky on a scene.
                uint16_t bg = cyberSkyAt(y0 + y * cell);
                tft.fillRect(x0 + x * cell, y0 + y * cell, cell, cell, animColor(ic, idx, x, y, t, bg));
            }
        } else if (animOnly) {
            pos += n;
        } else {
            while (n > 0) {
                int x = pos % CYI_W, y = pos / CYI_W, len = min(n, CYI_W - x);
                tft.fillRect(x0 + x * cell, y0 + y * cell, len * cell, cell, cyi_palette[idx]);
                pos += len;
                n -= len;
            }
        }
    }
}

struct Live {
    const CyberIcon *icon = nullptr;
    int cx, cy, cell;
    uint32_t last = 0;
} live;

// Option list layout, kept so a scroll only repaints rows.
struct ListState {
    bool header = false; // icon + title drawn for this screen
    String title;
} list;

const uint16_t DIM_DOT = 0x3088; // #301040, the design's unlit page dot

void title2(const String &text, int y, int room) {
    String t = text;
    t.toUpperCase();
    int size = 2;
    while (size > 1 && (int)t.length() * LW * size > room) size--;
    tft.setTextSize(size);
    cyberRainbowText(t, (tftWidth - t.length() * LW * size) / 2, y, cyberPhase(), bruceConfig.bgColor);
    tft.setTextSize(1);
}

} // namespace

bool cyberIcon(const char *menu, int cx, int cy, int cell, bool animate) {
    const CyberIcon *ic = findIcon(menu);
    if (!ic) return false;
    drawIcon(*ic, cx, cy, cell, cyberIconTick(), false);
    if (animate) live = {ic, cx, cy, cell, millis()};
    return true;
}

const char *cyberIconLabel(const char *menu) {
    const CyberIcon *ic = findIcon(menu);
    return ic ? ic->label : nullptr;
}

void cyberIconIdle() {
    if (!live.icon || millis() - live.last < 40) return;
    live.last = millis();
    drawIcon(*live.icon, live.cx, live.cy, live.cell, cyberIconTick(), true);
}

void cyberIconForget() {
    cyberSceneEnd();
    live.icon = nullptr;
    list.header = false;
}

bool cyberMainMenuItem(void *item, const String &menu) {
    const CyberIcon *ic = findIcon(menu.c_str());
    if (!ic) return false;
    // Position among the items actually shown (hidden menus are not in `options`).
    int index = 0, count = options.size();
    for (int i = 0; i < count; i++)
        if (options[i].hoverPointer == item) index = i;

    const uint16_t bg = bruceConfig.bgColor;
    const int cell = tftWidth >= 300 ? 3 : 2;
    const int iw = CYI_W * cell, ih = CYI_H * cell;
    const int areaTop = 27, areaBottom = tftHeight - 6;
    const int blockH = ih + 12 + 16 + 10 + 3;
    const int top = areaTop + (areaBottom - areaTop - blockH) / 2;
    const int cx = tftWidth / 2, cy = top + ih / 2;

    tft.fillRect(6, areaTop, tftWidth - 12, areaBottom - areaTop, bg);
    drawIcon(*ic, cx, cy, cell, cyberIconTick(), false);
    live = {ic, cx, cy, cell, millis()};

    // Magenta arrows, 18 px either side of the icon.
    const uint16_t mag = cyberHue(300);
    int lx = cx - iw / 2 - 18;
    tft.fillTriangle(lx - 9, cy, lx, cy - 9, lx, cy + 9, mag);
    int rx = cx + iw / 2 + 18;
    tft.fillTriangle(rx + 9, cy, rx, cy - 9, rx, cy + 9, mag);

    title2(ic->label, top + ih + 12, tftWidth - 24);

    // One dot per page, the current one lit in its own colour.
    const int dotY = top + ih + 12 + 16 + 10;
    int dx = cx - (count * 6 - 3) / 2;
    for (int i = 0; i < count; i++)
        tft.fillRect(dx + i * 6, dotY, 3, 3, i == index ? cyberItemColor(i, count) : DIM_DOT);
    return true;
}

void cyberOptionList(int index, std::vector<Option> &options, const char *title) {
    const uint16_t bg = bruceConfig.bgColor;
    const int n = options.size();
    const String t = title ? title : "";
    const CyberIcon *icon = headerIcon(title);
    if (!list.header || list.title != t) {
        drawStatusBar();
        tft.fillRect(6, 27, tftWidth - 12, tftHeight - 33, bg);
        if (t.length()) title2(t, BORDER_PAD_Y + 2, tftWidth - 112); // clear of "< back"
        tft.setTextSize(1);
        tft.setTextColor(TFT_DARKGREY, bg);
        tft.drawString("< back", 12, BORDER_PAD_Y + 6, 1); // top-left third is ESC on touch
        if (icon) {
            drawIcon(*icon, tftWidth / 2, 46 + CYI_H, 2, cyberIconTick(), false);
            live = {icon, tftWidth / 2, 46 + CYI_H, 2, millis()};
        }
        list.header = true;
        list.title = t;
    }

    const int top = icon ? 46 + CYI_H * 2 + 6 : 50;
    const int rowH = 16, x = 20, w = tftWidth - 40;
    const int rows = max(1, min(n, (tftHeight - 25 - top) / rowH)); // room for the counter above the frame
    const int first = max(0, min(index - rows / 2, n - rows));
    const int boxH = rows * rowH + 8;
    cyberRainbowFrame(x, top, w, boxH, cyberPhase());
    const int room = (w - 16) / LW;
    tft.setTextSize(1);
    for (int j = 0; j < rows; j++) {
        const int i = first + j, ry = top + 4 + j * rowH;
        const Option &o = options[i];
        const bool on = i == index;
        uint16_t c = o.enabled ? cyberItemColor(i, n) : TFT_DARKGREY;
        tft.fillRect(x + 2, ry, w - 4, rowH, on ? c : bg);
        tft.setTextColor(on ? TFT_BLACK : c, on ? c : bg);
        String label = o.label, value;
        int split = label.indexOf(": ");
        if (split > 0) {
            value = label.substring(split + 2);
            label = label.substring(0, split);
        }
        if (o.selected) value = value.length() ? value : "*";
        int maxLabel = room - 2 - (value.length() ? value.length() + 1 : 0);
        if ((int)label.length() > maxLabel) label = label.substring(0, max(1, maxLabel - 2)) + "..";
        tft.drawString((on ? "> " : "  ") + label, x + 8, ry + 4, 1);
        if (value.length()) {
            if ((int)value.length() > room / 2) value = value.substring(0, room / 2 - 2) + "..";
            tft.drawRightString(value, x + w - 8, ry + 4, 1);
        }
    }
    const int cy = top + boxH + 3;
    tft.fillRect(x, cy, w, 8, bg);
    if (n > rows) {
        tft.setTextColor(TFT_DARKGREY, bg);
        tft.drawCentreString(String(index + 1) + "/" + String(n), tftWidth / 2, cy, 1);
    }
#if defined(HAS_TOUCH)
    TouchFooter();
#endif
}
