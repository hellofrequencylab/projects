// Frequency's rainbow effects, drawn straight to the TFT (no frame buffer: the
// heap is needed for TLS when reader mode posts a scan).
#pragma once

#include <Arduino.h>

namespace freq {

// Three rainbow sine waves scrolling through a box: the "frequency" idle
// animation while waiting for a tap. Call once per frame with a rising t.
void fxWave(int x, int y, int w, int h, int t);

// Expanding rainbow rings and confetti from (cx, cy), ~0.8 s. Leaves the
// screen dirty: redraw afterwards.
void fxBurst(int cx, int cy);

// The boot scene's unicorn (apps/boot/unicorn_sprite.h), mane in rainbow.
void fxUnicorn(int x, int y, int scale, int frame, int phase);
constexpr int UNICORN_W = 40, UNICORN_H = 29;

// Stamp card: `required` circles (3-12), `from` already filled, animates
// filling up to `to`. Pops each new stamp with a sparkle.
void fxStampCard(int x, int y, int w, int required, int from, int to);

// Big centred rainbow text at text size `size`, clipped to the screen.
void fxCentreText(const String &text, int cy, int size, int phase);

} // namespace freq
