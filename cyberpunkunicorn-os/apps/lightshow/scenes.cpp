#include "scenes.h"

#include <math.h>
#include <string.h>

namespace lightshow {

namespace {

constexpr float C = (N - 1) / 2.0f;
constexpr float PI_F = 3.14159265f;

uint8_t SIN[256];
float DIST[N * N];
float ANG[N * N];
bool tablesReady = false;

void initTables() {
    if (tablesReady) return;
    for (int i = 0; i < 256; i++) SIN[i] = (uint8_t)(127.5f + 127.5f * sinf(i * PI_F / 128));
    for (int i = 0; i < N * N; i++) {
        float x = i % N - C, y = i / N - C;
        DIST[i] = sqrtf(x * x + y * y);
        ANG[i] = atan2f(y, x);
    }
    tablesReady = true;
}

uint32_t rngState = 0x5EED1234u;
uint32_t rnd32() {
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return rngState;
}
float frand() { return (rnd32() & 0xFFFFFF) / 16777216.0f; }             // [0,1)
float funiform(float a, float b) { return a + (b - a) * frand(); }
int irand(int a, int b) { return a + (int)(rnd32() % (uint32_t)(b - a + 1)); } // inclusive

float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
uint8_t u8(float v) { return v <= 0 ? 0 : v >= 255 ? 255 : (uint8_t)v; }

RGB rgb(int r, int g, int b) { return RGB{u8(r), u8(g), u8(b)}; }
RGB hex(uint32_t c) { return RGB{(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c}; }

RGB wheel(int pos) {
    pos &= 255;
    if (pos < 85) return rgb(255 - pos * 3, pos * 3, 0);
    if (pos < 170) {
        pos -= 85;
        return rgb(0, 255 - pos * 3, pos * 3);
    }
    pos -= 170;
    return rgb(pos * 3, 0, 255 - pos * 3);
}

RGB scale(RGB c, float k) { return RGB{u8(c.r * k), u8(c.g * k), u8(c.b * k)}; }

RGB mix(RGB a, RGB b, float k) {
    return RGB{u8(a.r + (b.r - a.r) * k), u8(a.g + (b.g - a.g) * k), u8(a.b + (b.b - a.b) * k)};
}

RGB hsv(float h, float s, float v) {
    h = fmodf(h, 1.0f);
    if (h < 0) h += 1.0f;
    h *= 6;
    int i = (int)h;
    float f = h - i;
    v = clamp01(v);
    float p = v * (1 - s), q = v * (1 - s * f), r = v * (1 - s * (1 - f));
    float o[3];
    switch (i % 6) {
        case 0: o[0] = v, o[1] = r, o[2] = p; break;
        case 1: o[0] = q, o[1] = v, o[2] = p; break;
        case 2: o[0] = p, o[1] = v, o[2] = r; break;
        case 3: o[0] = p, o[1] = q, o[2] = v; break;
        case 4: o[0] = r, o[1] = p, o[2] = v; break;
        default: o[0] = v, o[1] = p, o[2] = q; break;
    }
    return RGB{u8(o[0] * 255), u8(o[1] * 255), u8(o[2] * 255)};
}

// ---------- Mystic set: soft sound-healing -> sacred geometry -> cyberpunk ----------

class SingingBowl : public Scene {
public:
    const char *name() const override { return "Singing Bowl"; }
    float gamma() const override { return 1.6f; }
    void frame(float t, RGB *buf) override {
        float hue = 0.68f + 0.09f * sinf(t * 0.06f);
        for (int i = 0; i < N * N; i++) {
            float d = DIST[i];
            float w = 0.5f + 0.5f * sinf(d * 1.4f - t * 0.9f);
            buf[i] = hsv(hue + d * 0.012f, 0.75f, 0.14f + 0.86f * w * w);
        }
    }
};

class SoftRain : public Scene {
    static constexpr int DROPS = 8;
    struct Drop {
        int col;
        float y, speed, hue, bright;
    } drops[DROPS];
    bool started = false;
    float last = 0;

    Drop newDrop(float y, int self) {
        bool used[N] = {};
        for (int n = 0; n < DROPS; n++)
            if (n != self && (started || n < self)) used[drops[n].col] = true;
        int freeCols[N], nf = 0;
        for (int x = 0; x < N; x++)
            if (!used[x]) freeCols[nf++] = x;
        int col = nf ? freeCols[irand(0, nf - 1)] : irand(0, N - 1);
        return Drop{col, y, funiform(0.85f, 1.1f), funiform(0.56f, 0.66f), funiform(0.3f, 1.0f)};
    }

public:
    const char *name() const override { return "Soft Rain"; }
    float gamma() const override { return 1.8f; }
    void frame(float t, RGB *buf) override {
        float dt = fminf(0.2f, fmaxf(0.0f, t - last));
        last = t;
        if (!started) { // start spread down the panel so it never bunches up
            for (int n = 0; n < DROPS; n++) drops[n] = newDrop(N - n * (N + 2) / (float)DROPS, n);
            started = true;
        }
        RGB night = hsv(0.63f, 0.9f, 0.07f);
        for (int i = 0; i < N * N; i++) buf[i] = night;
        for (int n = 0; n < DROPS; n++) {
            Drop &d = drops[n];
            d.y += dt * 1.5f * d.speed;
            if (d.y >= N + 1) d = newDrop(-funiform(0, 1.5f), n);
            int y = (int)floorf(d.y);
            float tw = d.bright * (0.8f + 0.2f * sinf(t * 7 + n * 1.9f));
            if (y >= 0 && y < N) buf[y * N + d.col] = hsv(d.hue, 0.35f, tw);
            if (y - 1 >= 0 && y - 1 < N) buf[(y - 1) * N + d.col] = hsv(d.hue, 0.75f, 0.3f * tw);
        }
    }
};

class Prana : public Scene {
public:
    const char *name() const override { return "Prana Breath"; }
    float gamma() const override { return 1.6f; }
    void frame(float t, RGB *buf) override {
        float cyc = fmodf(t, 14.0f);
        float r, hue, v;
        if (cyc < 4) {
            float k = cyc / 4;
            r = 0.6f + 4.2f * k * k * (3 - 2 * k), hue = 0.47f, v = 0.65f;
        } else if (cyc < 7) {
            r = 4.8f, hue = 0.47f + 0.06f * (cyc - 4) / 3, v = 0.55f + 0.06f * sinf((cyc - 4) * 3);
        } else if (cyc < 12) {
            float k = (cyc - 7) / 5;
            r = 4.8f - 4.2f * k * k * (3 - 2 * k), hue = 0.95f, v = 0.6f;
        } else {
            r = 0.9f, hue = 0.98f, v = 0.25f;
        }
        for (int i = 0; i < N * N; i++) {
            float edge = clamp01((r - DIST[i]) / 1.2f + 0.5f);
            buf[i] = hsv(hue, 0.85f, 0.05f + v * edge * (1 - 0.35f * DIST[i] / 5));
        }
    }
};

class SeedOfLife : public Scene {
public:
    const char *name() const override { return "Seed of Life"; }
    float gamma() const override { return 1.9f; }
    void frame(float t, RGB *buf) override {
        const float r = 1.75f, a0 = t * 0.2f;
        float cx[7] = {0}, cy[7] = {0};
        for (int k = 0; k < 6; k++) {
            cx[k + 1] = r * cosf(a0 + k * 1.0472f);
            cy[k + 1] = r * sinf(a0 + k * 1.0472f);
        }
        for (int i = 0; i < N * N; i++) {
            float x = i % N - C, y = i / N - C;
            float best = 0, hue = 0.6f;
            for (int n = 0; n < 7; n++) {
                float dx = x - cx[n], dy = y - cy[n];
                float line = clamp01(1 - fabsf(sqrtf(dx * dx + dy * dy) - r) * 3.4f);
                if (line > best) best = line, hue = 0.5f + n * 0.045f;
            }
            float core = clamp01(1 - DIST[i] * 1.8f);
            buf[i] = mix(hsv(hue + 0.03f * sinf(t * 0.5f), 0.95f, 0.07f + 0.93f * best), RGB{255, 255, 255}, core);
        }
    }
};

class Lotus : public Scene {
public:
    const char *name() const override { return "Lotus Bloom"; }
    float gamma() const override { return 2.0f; }
    void frame(float t, RGB *buf) override {
        float bloom = 0.5f - 0.5f * cosf(t * 0.5f);
        float size = 1.6f + 2.6f * bloom, rot = t * 0.15f;
        for (int i = 0; i < N * N; i++) {
            float d = DIST[i];
            float petal = size * (0.45f + 0.55f * fabsf(cosf(4 * (ANG[i] - rot))));
            if (d < 0.95f) buf[i] = hsv(0.12f, 0.75f, 0.9f);
            else if (d < petal) {
                float k = d / petal;
                buf[i] = hsv(0.93f + 0.04f * k, 0.45f + 0.5f * k, 0.9f - 0.45f * k);
            } else buf[i] = RGB{14, 0, 10};
        }
    }
};

class GoldenSpiral : public Scene {
public:
    const char *name() const override { return "Golden Spiral"; }
    float gamma() const override { return 2.1f; }
    void frame(float t, RGB *buf) override {
        const int arms = 2;
        for (int i = 0; i < N * N; i++) {
            float d = DIST[i] + 0.3f;
            float v = 0.5f + 0.5f * cosf(arms * ANG[i] - logf(d) * 4.2f + t * 1.5f);
            v = v * v * v;
            buf[i] = hsv(0.08f + 0.05f * v, 0.95f, v * fminf(1.0f, d / 1.2f));
        }
    }
};

class Metatron : public Scene {
public:
    const char *name() const override { return "Metatron"; }
    float gamma() const override { return 2.2f; }
    void frame(float t, RGB *buf) override {
        const float r = 2.0f;
        float nx[2][3], ny[2][3];
        float starts[2] = {t * 0.25f - PI_F / 2, -t * 0.25f + PI_F / 2};
        for (int s = 0; s < 2; s++)
            for (int k = 0; k < 3; k++) {
                nx[s][k] = cosf(starts[s] + k * 2.0944f);
                ny[s][k] = sinf(starts[s] + k * 2.0944f);
            }
        const RGB colors[2] = {{255, 170, 0}, {0, 200, 255}};
        for (int i = 0; i < N * N; i++) {
            float x = i % N - C, y = i / N - C;
            RGB out{0, 0, 0};
            for (int s = 0; s < 2; s++) {
                float edge = -1e9f;
                for (int k = 0; k < 3; k++) edge = fmaxf(edge, x * nx[s][k] + y * ny[s][k]);
                float line = clamp01(1 - fabsf(edge - r) * 2.2f);
                if (line > 0) {
                    RGB c = scale(colors[s], line);
                    out = RGB{max8(out.r, c.r), max8(out.g, c.g), max8(out.b, c.b)};
                }
            }
            if (DIST[i] < 0.8f) out = RGB{255, 255, 255};
            buf[i] = out;
        }
    }

private:
    static uint8_t max8(uint8_t a, uint8_t b) { return a > b ? a : b; }
};

// Chakra motions: brightness 0..1 (crown sparkles return 1.5).
float chRoot(int i, float t) {
    float x = i % N - C, y = i / N - C;
    float v = 0.5f + 0.5f * cosf(fmaxf(fabsf(x), fabsf(y)) * 1.9f + t * 1.2f);
    return v * v;
}
float chSacral(int i, float t) {
    float x = i % N, y = i / N;
    float v = 0.5f + 0.5f * sinf(x * 0.9f + sinf(y * 0.8f + t * 0.9f) * 1.6f + t * 1.4f);
    return v * v;
}
float chSolar(int i, float t) {
    float rays = fmaxf(0.0f, cosf(8 * ANG[i] - t * 1.5f));
    return fminf(1.0f, rays * rays * 0.8f + clamp01(1.8f - DIST[i]));
}
float chHeart(int i, float t) {
    float beat = fmodf(t, 1.6f);
    float s = 1.0f + ((beat < 0.15f || (beat > 0.3f && beat < 0.45f)) ? 0.18f : 0.0f);
    float x = (i % N - C) / (2.6f * s);
    float y = -(i / N - C - 0.3f) / (2.6f * s);
    float a = x * x + y * y - 1;
    float v = a * a * a - x * x * y * y * y;
    return v <= 0 ? 1.0f : 0.0f;
}
float chThroat(int i, float t) {
    float x = fabsf(i % N - C), y = fabsf(i / N - C);
    float v = 0.5f + 0.5f * sinf(x * 1.7f - t * 3);
    return v * v * clamp01(1.3f - y / 3.5f);
}
float chBrow(int i, float t) {
    float r = 1.6f + 0.9f * sinf(t * 1.1f);
    return clamp01(1 - fabsf(DIST[i] - r) * 1.6f) + (DIST[i] > 0.8f ? 0.0f : 0.3f);
}
float chCrown(int i, float t) {
    float v = 0.5f + 0.5f * cosf(3 * ANG[i] + DIST[i] * 1.4f - t * 1.6f);
    return frand() < 0.025f ? 1.5f : v * v * v;
}

struct ChakraDef {
    const char *name;
    RGB col;
    float (*fn)(int, float);
};
const ChakraDef CHAKRAS[7] = {
    {"Root",      {255, 0, 0},   chRoot  },
    {"Sacral",    {255, 60, 0},  chSacral},
    {"Solar",     {255, 170, 0}, chSolar },
    {"Heart",     {0, 255, 50},  chHeart },
    {"Throat",    {0, 110, 255}, chThroat},
    {"Third Eye", {70, 0, 255},  chBrow  },
    {"Crown",     {190, 0, 255}, chCrown },
};

class Chakra : public Scene {
    const char *current = "Root";

public:
    const char *name() const override { return "Chakra"; }
    const char *label() const override { return current; }
    void frame(float t, RGB *buf) override {
        const float each = 9;
        float pos = t / each;
        int k = (int)pos % 7;
        float fade = clamp01((pos - (int)pos - 0.85f) / 0.15f);
        const ChakraDef &a = CHAKRAS[k], &b = CHAKRAS[(k + 1) % 7];
        current = a.name;
        for (int i = 0; i < N * N; i++) {
            float va = a.fn(i, t);
            RGB ca = va > 1.2f ? RGB{255, 230, 255} : scale(a.col, va); // u8() clamps like min(255, ...)
            if (fade > 0) {
                float vb = b.fn(i, t);
                ca = mix(ca, scale(b.col, vb), fade);
            }
            buf[i] = ca;
        }
    }
};

class ThirdEye : public Scene {
public:
    const char *name() const override { return "Third Eye"; }
    float gamma() const override { return 2.1f; }
    void frame(float t, RGB *buf) override {
        float pulse = 0.5f + 0.5f * sinf(t * 2.2f);
        for (int i = 0; i < N * N; i++) {
            float d = DIST[i];
            float rays = fmaxf(0.0f, cosf(6 * ANG[i] - t * 1.4f));
            rays = rays * rays * rays;
            float core = clamp01(1.3f + pulse * 0.8f - d);
            RGB ray = hsv(0.73f, 1.0f, rays * clamp01(0.4f + d / 3));
            buf[i] = mix(ray, mix(RGB{70, 0, 255}, RGB{255, 255, 255}, pulse * 0.6f), core);
        }
    }
};

class DataStream : public Scene {
    float drops[N], rates[N];
    bool started = false;
    float last = 0;

public:
    const char *name() const override { return "Data Stream"; }
    float gamma() const override { return 2.1f; }
    void frame(float t, RGB *buf) override {
        if (!started) {
            for (int x = 0; x < N; x++) drops[x] = funiform(-8, 8), rates[x] = funiform(0.6f, 1.3f);
            started = true;
        }
        float dt = fminf(0.2f, fmaxf(0.0f, t - last));
        last = t;
        memset(buf, 0, sizeof(RGB) * N * N);
        static const uint8_t TRAIL[5] = {230, 150, 90, 45, 18};
        for (int x = 0; x < N; x++) {
            drops[x] += dt * 7 * rates[x];
            if (drops[x] > N + 6) drops[x] = funiform(-6, 0), rates[x] = funiform(0.6f, 1.3f);
            int head = (int)floorf(drops[x]);
            for (int tail = 0; tail < 6; tail++) {
                int y = head - tail;
                if (y < 0 || y >= N) continue;
                RGB c;
                if (tail == 0) c = RGB{180, 255, 180};
                else {
                    uint8_t g = TRAIL[tail - 1];
                    c = RGB{0, (uint8_t)(frand() > 0.06f ? g : g / 3), 0};
                }
                buf[(N - 1 - y) * N + x] = c; // upside down, so it streams upward
            }
        }
    }
};

class Synthwave : public Scene {
public:
    const char *name() const override { return "Synthwave"; }
    float gamma() const override { return 2.0f; }
    void frame(float t, RGB *buf) override {
        const float sp = 1.2f;
        float scroll = fmodf(t * 0.7f * sp, 1.0f);
        bool nearLine = fmodf(t * 1.4f * sp, 1.0f) > 0.5f;
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++) {
                int i = y * N + x;
                if (y <= 4) {
                    float dx = (x - C) * 1.25f, dy = y - 4.8f;
                    if (sqrtf(dx * dx + dy * dy) < 4.3f) {
                        bool gap = y >= 3 && fmodf(y + scroll, 2.0f) < 0.6f;
                        buf[i] = gap ? RGB{0, 0, 0} : mix(RGB{255, 220, 0}, RGB{255, 0, 120}, y / 4.0f);
                    } else buf[i] = RGB{25, 0, 45};
                } else if (y == 5) buf[i] = RGB{255, 0, 180};
                else {
                    bool line = nearLine ? (y == 7) : (y == 6);
                    bool vert = (y == 6) ? (x == 2 || x == 5) : (x == 0 || x == 3 || x == 4 || x == 7);
                    buf[i] = line ? RGB{0, 200, 255} : vert ? RGB{0, 90, 140} : RGB{15, 0, 30};
                }
            }
    }
};

// ---------- Classic scenes ----------

class Rainbow : public Scene {
public:
    const char *name() const override { return "Rainbow"; }
    void frame(float t, RGB *buf) override {
        int base = (int)(t * 60);
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++) buf[y * N + x] = wheel(base + (x + y) * 12);
    }
};

class Plasma : public Scene {
public:
    const char *name() const override { return "Plasma"; }
    void frame(float t, RGB *buf) override {
        int k = (int)(t * 40);
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++) {
                int v = SIN[(x * 32 + k) & 255] + SIN[(y * 28 - k) & 255] + SIN[((x + y) * 20 + k * 2) & 255];
                buf[y * N + x] = wheel(v / 3 + k);
            }
    }
};

class Fire : public Scene {
    uint8_t heat[N][N] = {};
    float last = -1;

public:
    const char *name() const override { return "Fire"; }
    void frame(float t, RGB *buf) override {
        if (t - last >= 0.05f || last < 0) {
            last = t;
            const int cool = 55;
            for (int x = 0; x < N; x++) {
                for (int y = 0; y < N; y++) {
                    int v = heat[y][x] - irand(0, cool * 10 / N + 2);
                    heat[y][x] = v < 0 ? 0 : v;
                }
                for (int y = 0; y < N - 1; y++) {
                    int y2 = y + 2 < N ? y + 2 : N - 1;
                    heat[y][x] = (heat[y + 1][x] + heat[y + 1][x] + heat[y2][x]) / 3;
                }
                if (irand(0, 255) < 160) {
                    int v = heat[N - 1][x] + irand(120, 255);
                    heat[N - 1][x] = v > 255 ? 255 : v;
                }
            }
        }
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++) {
                int v = heat[y][x];
                buf[y * N + x] = v < 85 ? rgb(v * 3, 0, 0) : v < 170 ? rgb(255, (v - 85) * 3, 0) : rgb(255, 255, (v - 170) * 3);
            }
    }
};

class Sparkle : public Scene {
    float level[N * N] = {};
    float last = 0;

public:
    const char *name() const override { return "Sparkle"; }
    void frame(float t, RGB *buf) override {
        float dt = fminf(0.2f, fmaxf(0.0f, t - last));
        last = t;
        for (int n = 0; n < 4; n++)
            if (frand() < 0.5f) level[irand(0, N * N - 1)] = 1.0f;
        const RGB fg = hex(0xFFFFFF), bg = hex(0x100018);
        for (int i = 0; i < N * N; i++) {
            buf[i] = mix(bg, fg, level[i]);
            level[i] = fmaxf(0.0f, level[i] - dt * 2.5f);
        }
    }
};

class Rain : public Scene {
    float drops[N];
    bool started = false;
    float last = 0;

public:
    const char *name() const override { return "Rain"; }
    void frame(float t, RGB *buf) override {
        if (!started) {
            for (int x = 0; x < N; x++) drops[x] = funiform(-8, 8);
            started = true;
        }
        float dt = fminf(0.2f, fmaxf(0.0f, t - last));
        last = t;
        const RGB col = hex(0x00FF40);
        static const float TAIL[4] = {1.0f, 0.45f, 0.2f, 0.08f};
        memset(buf, 0, sizeof(RGB) * N * N);
        for (int x = 0; x < N; x++) {
            drops[x] += dt * 9 * (0.6f + 0.4f * ((x * 7) % 3));
            if (drops[x] > N + 4) drops[x] = funiform(-6, 0);
            int head = (int)floorf(drops[x]);
            for (int tail = 0; tail < 4; tail++) {
                int y = head - tail;
                if (y >= 0 && y < N) buf[y * N + x] = scale(col, TAIL[tail]);
            }
        }
    }
};

class Breathe : public Scene {
public:
    const char *name() const override { return "Breathe"; }
    void frame(float t, RGB *buf) override {
        float k = 0.08f + 0.92f * SIN[(int)(t * 50) & 255] / 255.0f;
        RGB px = scale(hex(0xFF2080), k);
        for (int i = 0; i < N * N; i++) buf[i] = px;
    }
};

// 5x7 font, one byte per column, bit 0 = top row ('<' is a heart).
struct Glyph {
    char ch;
    uint8_t cols[5];
};
const Glyph FONT[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}}, {'!', {0x00, 0x00, 0x5f, 0x00, 0x00}}, {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}}, {'?', {0x02, 0x01, 0x59, 0x09, 0x06}}, {'<', {0x0c, 0x1e, 0x3c, 0x1e, 0x0c}},
    {'0', {0x3e, 0x51, 0x49, 0x45, 0x3e}}, {'1', {0x00, 0x42, 0x7f, 0x40, 0x00}}, {'2', {0x72, 0x49, 0x49, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x49, 0x4d, 0x33}}, {'4', {0x18, 0x14, 0x12, 0x7f, 0x10}}, {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3c, 0x4a, 0x49, 0x49, 0x31}}, {'7', {0x41, 0x21, 0x11, 0x09, 0x07}}, {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x46, 0x49, 0x49, 0x29, 0x1e}}, {'A', {0x7c, 0x12, 0x11, 0x12, 0x7c}}, {'B', {0x7f, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3e, 0x41, 0x41, 0x41, 0x22}}, {'D', {0x7f, 0x41, 0x41, 0x41, 0x3e}}, {'E', {0x7f, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7f, 0x09, 0x09, 0x09, 0x01}}, {'G', {0x3e, 0x41, 0x41, 0x51, 0x73}}, {'H', {0x7f, 0x08, 0x08, 0x08, 0x7f}},
    {'I', {0x00, 0x41, 0x7f, 0x41, 0x00}}, {'J', {0x20, 0x40, 0x41, 0x3f, 0x01}}, {'K', {0x7f, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7f, 0x40, 0x40, 0x40, 0x40}}, {'M', {0x7f, 0x02, 0x1c, 0x02, 0x7f}}, {'N', {0x7f, 0x04, 0x08, 0x10, 0x7f}},
    {'O', {0x3e, 0x41, 0x41, 0x41, 0x3e}}, {'P', {0x7f, 0x09, 0x09, 0x09, 0x06}}, {'Q', {0x3e, 0x41, 0x51, 0x21, 0x5e}},
    {'R', {0x7f, 0x09, 0x19, 0x29, 0x46}}, {'S', {0x26, 0x49, 0x49, 0x49, 0x32}}, {'T', {0x03, 0x01, 0x7f, 0x01, 0x03}},
    {'U', {0x3f, 0x40, 0x40, 0x40, 0x3f}}, {'V', {0x1f, 0x20, 0x40, 0x20, 0x1f}}, {'W', {0x3f, 0x40, 0x38, 0x40, 0x3f}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}}, {'Y', {0x03, 0x04, 0x78, 0x04, 0x03}}, {'Z', {0x61, 0x59, 0x49, 0x4d, 0x43}},
};

const uint8_t *glyph(char ch) {
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    for (const Glyph &g : FONT)
        if (g.ch == ch) return g.cols;
    return glyph('?');
}

class Text : public Scene {
    static constexpr const char *TEXT = "CYBERUNICORN 5000 <";
    static constexpr int MAXCOLS = 160;
    uint8_t cols[MAXCOLS];
    int ncols = 0;

public:
    const char *name() const override { return "Text"; }
    void frame(float t, RGB *buf) override {
        if (!ncols) {
            for (int i = 0; i < N; i++) cols[ncols++] = 0; // start off-screen right
            for (const char *p = TEXT; *p && ncols < MAXCOLS - 8; p++) {
                const uint8_t *g = glyph(*p);
                for (int c = 0; c < 5; c++) cols[ncols++] = g[c];
                cols[ncols++] = 0;
            }
            cols[ncols++] = 0;
            cols[ncols++] = 0;
        }
        int off = (int)(t * 8) % ncols;
        for (int x = 0; x < N; x++) {
            uint8_t bits = cols[(off + x) % ncols];
            RGB color = wheel((int)(t * 40) + x * 10);
            for (int y = 0; y < N; y++) buf[y * N + x] = (bits >> y) & 1 ? color : RGB{0, 0, 0};
        }
    }
};

Scene *ALL[18];
int allCount = 0;

void initScenes() {
    if (allCount) return;
    initTables();
    rngState ^= (uint32_t)(uintptr_t)&rngState; // a little variety between boots
    Scene *list[] = {
        new SingingBowl(), new SoftRain(),  new Prana(),     new SeedOfLife(), new Lotus(),   new GoldenSpiral(),
        new Metatron(),    new Chakra(),    new ThirdEye(),  new DataStream(), new Synthwave(),
        new Rainbow(),     new Plasma(),    new Fire(),      new Sparkle(),    new Rain(),    new Breathe(),
        new Text(),
    };
    for (Scene *s : list) ALL[allCount++] = s;
}

} // namespace

int sceneCount() {
    initScenes();
    return allCount;
}

Scene *scene(int index) {
    initScenes();
    index %= allCount;
    if (index < 0) index += allCount;
    return ALL[index];
}

} // namespace lightshow
