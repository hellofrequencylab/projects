#pragma once

#include <stdint.h>

// The cyberpunkunicorn OS boot scene: synthwave sky, striped sun, scrolling neon
// grid, a galloping rainbow unicorn and a loading bar that lies about progress.
// Runs for durationMs or until any key/touch. Returns false (and draws nothing)
// if there isn't enough RAM for the frame buffer, so the caller can fall back.
bool unicornBootScene(uint32_t durationMs);
