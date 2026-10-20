#include "SlideshowMenu.h"

#include "../ui/cyber_icon.h"

#include "../ui/rainbow.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <Preferences.h>
#include <algorithm>
#include <globals.h>
#include <stdarg.h>
#include <vector>

namespace {

const char *FOLDER = "/slides";
// Slide time choices, ms. The original's System > Settings offered 0.5/1/2/4 s.
const int SLIDE_MS[] = {500, 1000, 2000, 4000, 8000, 15000};
constexpr int SLIDE_MS_COUNT = sizeof(SLIDE_MS) / sizeof(SLIDE_MS[0]);
const uint32_t GIF_MAX_MS = 15000; // safety cap for very long GIFs (as in the original)
const uint32_t CAPTION_MS = 1800;
const size_t MAX_SLIDES = 200;

struct Settings {
    int slideMs = 1000; // the original's SLIDE_SECONDS default
    bool loop = true;   // the original always looped

    void load() {
        Preferences p;
        p.begin("slideshow", true);
        slideMs = p.getInt("ms", slideMs);
        loop = p.getBool("loop", loop);
        p.end();
        if (slideMs < 100 || slideMs > 60000) slideMs = 1000;
    }
    void save() const {
        Preferences p;
        p.begin("slideshow", false);
        p.putInt("ms", slideMs);
        p.putBool("loop", loop);
        p.end();
    }
};

String secondsLabel(int ms) {
    return ms % 1000 ? String(ms / 1000.0f, 1) + "s" : String(ms / 1000) + "s";
}

bool isSlide(String name) {
    name.toLowerCase();
    if (name.startsWith(".")) return false;
    return name.endsWith(".jpg") || name.endsWith(".jpeg") || name.endsWith(".png") ||
           name.endsWith(".bmp") || name.endsWith(".gif");
}

// Where the pictures are: SD /slides if present, else LittleFS /slides.
struct Source {
    FS *fs = nullptr;
    const char *where = "";
    std::vector<String> files;
};

Source findSlides() {
    Source s;
    if (!sdcardMounted) setupSdCard();
    FS *candidates[] = {sdcardMounted ? (FS *)&SD : nullptr, (FS *)&LittleFS};
    const char *names[] = {"SD", "LittleFS"};
    for (int c = 0; c < 2; c++) {
        FS *fs = candidates[c];
        if (!fs || !fs->exists(FOLDER)) continue;
        File dir = fs->open(FOLDER);
        if (!dir || !dir.isDirectory()) continue;
        std::vector<String> files;
        for (File f = dir.openNextFile(); f && files.size() < MAX_SLIDES; f = dir.openNextFile()) {
            String n = f.name();
            int slash = n.lastIndexOf('/');
            if (slash >= 0) n = n.substring(slash + 1);
            if (!f.isDirectory() && isSlide(n)) files.push_back(n);
            f.close();
        }
        dir.close();
        if (files.empty()) continue;
        std::sort(files.begin(), files.end());
        s.fs = fs;
        s.where = names[c];
        s.files = files;
        return s;
    }
    return s;
}

void emptyScreen() {
    drawMainBorderWithTitle("SLIDESHOW");
    tft.setTextSize(2);
    cyberRainbowText("No pictures yet", tftWidth / 2 - 90, 70, cyberPhase(), bruceConfig.bgColor);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.drawCentreString("Copy .jpg .png .bmp .gif files into", tftWidth / 2, 105, 1);
    tft.setTextColor(cyberHue(300), bruceConfig.bgColor);
    tft.drawCentreString("/slides on the SD card (FAT32)", tftWidth / 2, 120, 1);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("or /slides on LittleFS", tftWidth / 2, 135, 1);
    tft.drawCentreString("Best fit: 320x240 or smaller", tftWidth / 2, 160, 1);
    tft.drawCentreString("any key: back", tftWidth / 2, 190, 1);
    while (!check(AnyKeyPress)) delay(30);
}

// "3/10  name.gif" on a black strip along the bottom, with a rainbow rule above.
void drawCaption(const String &text) {
    const int h = 12, y = tftHeight - h;
    tft.fillRect(0, y, tftWidth, h, TFT_BLACK);
    cyberRainbowHLine(0, y - 1, tftWidth, cyberPhase());
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(text, tftWidth / 2, y + 2, 1);
}

enum class Input { None, Next, Prev, Esc };

Input pollInput() {
    if (check(EscPress)) return Input::Esc;
    if (check(NextPress)) return Input::Next;
    if (check(PrevPress)) return Input::Prev;
    if (check(SelPress)) return Input::Next; // a tap on the picture moves on too
    return Input::None;
}

// Plays the GIF through once. Returns the input that interrupted it, if any.
Input playGif(FS *fs, const String &path) {
    Gif gif;
    if (!gif.openGIF(fs, path.c_str())) return Input::None;
    int x = max(0, (tftWidth - gif.getCanvasWidth()) / 2);
    int y = max(0, (tftHeight - gif.getCanvasHeight()) / 2);
    uint32_t start = millis();
    while (true) {
        int r = gif.playFrame(x, y, true); // bSync: waits out the frame's own delay
        Input in = pollInput();
        if (in != Input::None) return in;
        if (r <= 0) return Input::None; // 0 = last frame played, -1 = decode error
        if (millis() - start > GIF_MAX_MS) return Input::None;
    }
}

bool drawStill(FS *fs, const String &path) {
    String lower = path;
    lower.toLowerCase();
    if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) {
        // showJpeg loads the whole file into RAM first: refuse what can't fit.
        File f = fs->open(path, FILE_READ);
        size_t size = f ? f.size() : 0;
        if (f) f.close();
        if (size == 0 || size + 16384 > ESP.getMaxAllocHeap()) return false;
        // drawImg() picks the decoder by extension and doesn't know ".jpeg".
        if (lower.endsWith(".jpeg")) return showJpeg(*fs, path, 0, 0, true);
    }
    return drawImg(*fs, path, 0, 0, true);
}

} // namespace

void SlideshowMenu::optionsMenu() {
    Settings s;
    s.load();
    options = {
        {"Start slideshow", [this]() { run(); }},
        {("Slide time: " + secondsLabel(s.slideMs)).c_str(),
         [s]() mutable {
             int i = 0;
             while (i < SLIDE_MS_COUNT && SLIDE_MS[i] <= s.slideMs) i++;
             s.slideMs = SLIDE_MS[i % SLIDE_MS_COUNT];
             s.save();
         }},
        {(String("Loop: ") + (s.loop ? "forever" : "once")).c_str(),
         [s]() mutable {
             s.loop = !s.loop;
             s.save();
         }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Slideshow");
}

void SlideshowMenu::run() {
    Settings s;
    s.load();
    displayInfo("Looking for /slides...");
    Source src = findSlides();
    if (src.files.empty()) {
        emptyScreen();
        returnToMenu = true;
        return;
    }

    static int lastIndex = 0; // like the original, come back to where we left off
    const int count = src.files.size();
    int i = ((lastIndex % count) + count) % count;
    bool done = false;
    while (!done) {
        const String &name = src.files[i];
        const String path = String(FOLDER) + "/" + name;
        String lower = name;
        lower.toLowerCase();
        const bool gif = lower.endsWith(".gif");
        const String caption = String(i + 1) + "/" + String(count) + "  " + name;

        tft.fillScreen(TFT_BLACK);
        Input in = Input::None;
        if (gif) {
            drawCaption(caption); // GIF frames paint over it where they overlap
            in = playGif(src.fs, path);
        } else if (!drawStill(src.fs, path)) {
            drawCaption("skip " + name + ": can't decode / too big");
            uint32_t until = millis() + 3000;
            while (in == Input::None && millis() < until) in = pollInput(), delay(20);
        } else {
            drawCaption(caption);
            uint32_t shown = millis();
            bool captionUp = true;
            while (in == Input::None && millis() - shown < (uint32_t)s.slideMs) {
                in = pollInput();
                // Take the caption off by redrawing the picture, if it stays up long enough.
                if (captionUp && millis() - shown > CAPTION_MS && s.slideMs - (millis() - shown) > 1000) {
                    captionUp = false;
                    tft.fillRect(0, tftHeight - 13, tftWidth, 13, TFT_BLACK);
                    drawStill(src.fs, path);
                }
                delay(20);
            }
        }

        switch (in) {
            case Input::Esc: done = true; break;
            case Input::Prev: i = (i + count - 1) % count; break;
            default:
                if (i + 1 >= count && !s.loop) {
                    done = true;
                    i = 0;
                } else {
                    i = (i + 1) % count;
                }
        }
        lastIndex = i;
    }
    returnToMenu = true;
}

void SlideshowMenu::drawIcon(float scale) {
    // Pixel-art icon from design/icons (apps/ui/cyber_icons.h).
    clearIconArea();
    cyberIcon(getName().c_str(), iconCenterX, iconCenterY, max(2, (int)(scale * 2.4f + 0.5f)), true);
}

// Bruce 1.16.1's serialDevice->printf mangles its arguments (CLAUDE.md, gotcha 9):
// format here, then print the finished string.
static void sayf(const char *fmt, ...) {
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    serialDevice->print(buf);
}

static uint32_t slidesCallback(cmd *c) {
    Settings s;
    s.load();
    Source src = findSlides();
    sayf(
        "Slideshow: %s per still, %s\n", secondsLabel(s.slideMs).c_str(), s.loop ? "loops" : "plays once"
    );
    if (src.files.empty()) {
        serialDevice->println("No pictures in /slides on SD or LittleFS (.jpg .jpeg .png .bmp .gif)");
        return true;
    }
    sayf("%d slides in %s:%s\n", (int)src.files.size(), src.where, FOLDER);
    for (const String &n : src.files) serialDevice->println("  " + n);
    return true;
}

void createSlideshowCommand(SimpleCLI *cli) { cli->addCommand("slides", slidesCallback); }
