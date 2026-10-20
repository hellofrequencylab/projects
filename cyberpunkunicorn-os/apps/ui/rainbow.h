// CyberUnicorn 5000 skin: rainbow drawing helpers. Patch 0008 calls these from
// Bruce's shared drawing code (borders, titles, menus, stripes, progress bars,
// footer), so every app picks up the skin without being edited itself.
#pragma once

#include <Arduino.h>

// Full-saturation hue (degrees, any integer) -> RGB565.
uint16_t cyberHue(int hue);

// A distinct neon colour for item `index` of `count` (main menu, option lists).
uint16_t cyberItemColor(int index, int count);

// Hue offset that drifts with time, so redraws shimmer.
int cyberPhase();

// Rectangle outline whose colour walks the rainbow around the perimeter.
void cyberRainbowFrame(int x, int y, int w, int h, int phase);

// Horizontal line fading through the rainbow left to right.
void cyberRainbowHLine(int x, int y, int w, int phase);

// Filled bar (progress bars, highlights) with a rainbow sweep.
void cyberRainbowFill(int x, int y, int w, int h, int phase);

// Text with each letter a different hue. Uses the current text size.
// Returns the drawn width in pixels.
int cyberRainbowText(const String &text, int x, int y, int phase, uint16_t bg);
