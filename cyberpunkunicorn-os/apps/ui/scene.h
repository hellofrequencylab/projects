// CyberUnicorn 5000 design system, the pieces apps draw with (design/): the
// synthwave scene, glass panels, twinkling sparkles, confetti, the DIP-switch
// drawing and the hardware guide card. Straight to the TFT, no frame buffer:
// animations erase what they drew with the sky colour of that row, and skip
// anything registered with cyberKeepOut() (panels, icons, text).
#pragma once

#include <Arduino.h>

// Content area under the status bar and inside the rainbow border.
constexpr int CY_TOP = 46;      // first content row below a size-2 title
constexpr uint16_t CY_GLASS = 0x0803; // panel fill, the scene's floor colour (12,0,28)
int cyFooterY();                // grey key-hint row, just above the border

// Sky gradient, optional striped sun sitting on the horizon, black floor with
// a rainbow horizon line and a magenta perspective grid. Clears keep-outs and
// sparkles. `sunR` 0 = no sun.
void cyberScene(int horizon, int sunX = 160, int sunR = 36);
// The screen was cleared (drawMainBorder): no scene until the next cyberScene().
void cyberSceneEnd();
// Colour behind (x, y) on the scene: sky gradient above the horizon, black below.
uint16_t cyberSkyAt(int y);

// Areas animations must not paint over (up to 8 per scene).
void cyberKeepOut(int x, int y, int w, int h);
// True when a box touches a keep-out, the sun or leaves the sky.
bool cyberBlocked(int x, int y, int w, int h);

// Glass panel: floor-coloured fill, rainbow hairline. Registers a keep-out.
void cyberPanel(int x, int y, int w, int h);

// Rainbow title (size 2, scaled down to fit), centred at row y. Keep-out too.
void cyberHeadline(const String &text, int y, int size = 2);

// Grey key hint on the footer row: "SEL menu   ESC exit".
void cyberFooter(const String &text);

// Grey label at x, value at x + 68 in `color`, on the glass. Clips the value.
void cyberLine(int x, int y, const String &label, const String &value, uint16_t color = 0xFFFF);

// One animation step for everything that twinkles. Call every pass of a wait
// loop: it throttles itself to the 70 ms sparkle tick and 40 ms icon tick.
// `count` stars between the top and `bottom` (default: the horizon).
void cyberSparkles(int count = 26, int seed = 7, int bottom = 0);
// Confetti fountain from (cx, cy) on every step until cyberBurst(-1, -1).
void cyberBurst(int cx, int cy);
void cyberAnimate();

// Hat DIP switch, 1-6: `onMask` bits drawn up and lit, `offMask` drawn down,
// the rest dim. Bit 0 = switch 1. 184 x 62 px.
void cyberDipSwitch(int x, int y, uint8_t onMask, uint8_t offMask);

// Full-screen guide for an app opened with the wrong hat module live:
// instruction ("TURN DIP 3 ON"), DIP drawing, module + pins, three buttons.
// Returns 0 Check again, 1 Open anyway, 2 Back (also ESC).
int cyberHardwareCard(
    const String &title, const String &instruction, const String &module, const String &pins, uint8_t onMask,
    uint8_t offMask
);
