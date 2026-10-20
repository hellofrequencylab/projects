// Light Show scenes for an 8x8 LED panel, ported from the CircuitPython Unicorn OS
// launcher (scenes.py, 2026-10-17). Each scene fills 64 RGB pixels, row by row from
// the top-left; nothing here touches hardware.
#pragma once

#include <stdint.h>

namespace lightshow {

constexpr int N = 8;

struct RGB {
    uint8_t r, g, b;
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual const char *name() const = 0;
    virtual float gamma() const { return 2.2f; }
    // t is the scene clock in seconds. buf has N*N pixels.
    virtual void frame(float t, RGB *buf) = 0;
    // Optional sub-label (e.g. which chakra is showing); nullptr for none.
    virtual const char *label() const { return nullptr; }
};

// All scenes in display order: the soft-to-cyberpunk "mystic" set first, then the
// classics. Allocated on first call, kept for the session.
int sceneCount();
Scene *scene(int index);

} // namespace lightshow
