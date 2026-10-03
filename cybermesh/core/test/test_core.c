/* Host tests for the floor v0 core: SHA-256 vectors, object codec, frame
 * validation, and a simulated multi-node radio mesh. Build and run: make test */
#include "cm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static void hex(const uint8_t *d, size_t n, char *out)
{
    for (size_t i = 0; i < n; i++) sprintf(out + 2 * i, "%02x", d[i]);
}

static void test_sha256(void)
{
    struct { const char *in; const char *out; } v[] = {
        {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
        {"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
         "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"},
    };
    for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); i++) {
        uint8_t d[32];
        char h[65];
        cm_sha256((const uint8_t *)v[i].in, strlen(v[i].in), d);
        hex(d, 32, h);
        CHECK(strcmp(h, v[i].out) == 0);
    }
    static uint8_t mil[1000000];
    memset(mil, 'a', sizeof(mil));
    uint8_t d[32];
    char h[65];
    cm_sha256(mil, sizeof(mil), d);
    hex(d, 32, h);
    CHECK(strcmp(h, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0") == 0);
}

static void test_codec(void)
{
    cm_note a = {0}, b;
    a.author = 0xA1B2C3D4;
    a.time_ms = 1759500000000ULL;
    a.text_len = 5;
    memcpy(a.text, "hello", 5);
    uint8_t buf[CM_OBJ_MAX];
    size_t len = cm_note_encode(&a, buf);
    CHECK(len == 21);
    CHECK(buf[3] == 0xD4 && buf[6] == 0xA1); /* little-endian author */
    CHECK(cm_note_decode(buf, len, &b));
    CHECK(b.author == a.author && b.time_ms == a.time_ms && strcmp(b.text, "hello") == 0);

    /* The ID is SHA-256 of exactly these bytes: a fixed vector pins the format. */
    uint8_t d[32];
    cm_sha256(buf, len, d);
    CHECK(memcmp(d, b.short_id, CM_SHORT_ID) == 0);

    CHECK(!cm_note_decode(buf, len - 1, &b));  /* truncated */
    buf[15] = 0;
    CHECK(!cm_note_decode(buf, 16, &b));       /* empty text */
    buf[15] = 5;
    buf[1] = 0x01;
    CHECK(!cm_note_decode(buf, len, &b));      /* unknown suite */
}

/* ---- simulated radio ---- */

#define MAXN 8
typedef struct { int from; size_t len; uint8_t f[CM_FRAME_MAX]; } frame_t;

static cm_node nodes[MAXN];
static int nn;
static bool link_ok[MAXN][MAXN];
static uint32_t clock_ms;
static frame_t bus[4096];
static int bus_len;
static unsigned loss_pct;
static uint32_t rng = 12345;
static uint32_t total_tx;

static uint32_t xr(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static uint32_t sim_now(void *ctx) { (void)ctx; return clock_ms; }
static uint32_t sim_rand(void *ctx) { (void)ctx; return xr(); }
static bool sim_send(void *ctx, const uint8_t *f, size_t len)
{
    if (bus_len == (int)(sizeof(bus) / sizeof(bus[0]))) return false;
    bus[bus_len].from = (int)(intptr_t)ctx;
    bus[bus_len].len = len;
    memcpy(bus[bus_len].f, f, len);
    bus_len++;
    total_tx++;
    return true;
}

static void sim_init(int count, unsigned loss)
{
    nn = count;
    loss_pct = loss;
    clock_ms = 1000;
    bus_len = 0;
    total_tx = 0;
    memset(link_ok, 0, sizeof(link_ok));
    for (int i = 0; i < nn; i++) {
        cm_platform p = {sim_send, sim_now, sim_rand, NULL, (void *)(intptr_t)i};
        cm_node_init(&nodes[i], 0x1000u + (uint32_t)i, &p);
    }
}

static void sim_link(int a, int b) { link_ok[a][b] = link_ok[b][a] = true; }

static void sim_step(void)
{
    clock_ms += CM_TICK_MS;
    for (int i = 0; i < nn; i++) cm_node_tick(&nodes[i]);
    /* Deliver until quiet; frames sent while handling rx go out in the same step. */
    for (int k = 0; k < bus_len; k++) {
        frame_t fr = bus[k];
        for (int j = 0; j < nn; j++)
            if (link_ok[fr.from][j] && xr() % 100 >= loss_pct) cm_node_rx(&nodes[j], fr.f, fr.len);
    }
    bus_len = 0;
}

static bool converged(void)
{
    for (int i = 1; i < nn; i++)
        if (nodes[i].count != nodes[0].count || nodes[i].fingerprint != nodes[0].fingerprint) return false;
    return true;
}

static int run_until_converged(int max_steps)
{
    for (int s = 0; s < max_steps; s++) {
        sim_step();
        if (converged()) return s;
    }
    return -1;
}

static void post(int i, const char *t) { CHECK(cm_node_post(&nodes[i], 0, t, strlen(t))); }

static void test_two_boxes_one_note(void)
{
    sim_init(2, 0);
    sim_link(0, 1);
    post(0, "hello from box A");
    int steps = run_until_converged(400);
    CHECK(steps >= 0);
    CHECK(nodes[1].count == 1 && strcmp(nodes[1].store[0].text, "hello from box A") == 0);
    CHECK(nodes[1].store[0].author == nodes[0].id);
    printf("  two boxes, one note: %d ms\n", steps * (int)CM_TICK_MS);
}

static void test_three_hops(void)
{
    sim_init(4, 0); /* A - B - C - D, A cannot hear C or D */
    sim_link(0, 1);
    sim_link(1, 2);
    sim_link(2, 3);
    post(0, "relay me");
    int steps = run_until_converged(1000);
    CHECK(steps >= 0);
    CHECK(nodes[3].count == 1);
    printf("  three hops: %d ms\n", steps * (int)CM_TICK_MS);
}

static void test_bulk_lossy(void)
{
    sim_init(3, 20); /* 20% frame loss on every link */
    sim_link(0, 1);
    sim_link(1, 2);
    char t[40];
    for (int i = 0; i < 100; i++) {
        snprintf(t, sizeof(t), "note %d from A", i);
        post(0, t);
    }
    for (int i = 0; i < 20; i++) {
        snprintf(t, sizeof(t), "note %d from C", i);
        post(2, t);
    }
    int steps = run_until_converged(20000);
    CHECK(steps >= 0);
    CHECK(nodes[0].count == 120 && nodes[1].count == 120 && nodes[2].count == 120);
    printf("  120 notes, 3 nodes, 20%% loss: %d ms, %u frames\n", steps * (int)CM_TICK_MS, total_tx);
}

static void test_garbage_rejected(void)
{
    sim_init(1, 0);
    uint8_t junk[CM_FRAME_MAX];
    for (int i = 0; i < 2000; i++) {
        size_t len = xr() % (CM_FRAME_MAX + 1);
        for (size_t k = 0; k < len; k++) junk[k] = (uint8_t)xr();
        if (len >= 3 && i % 2) { junk[0] = 0x43; junk[1] = 0x4D; junk[2] = 0; } /* valid magic */
        cm_node_rx(&nodes[0], junk, len);
    }
    CHECK(nodes[0].count == 0);

    /* A self-sourced frame is dropped. */
    uint8_t own[18] = {0x43, 0x4D, 0, 0x01};
    memcpy(own + 4, &nodes[0].id, 4); /* host is little-endian in CI */
    uint32_t dropped = nodes[0].stats.frames_dropped;
    cm_node_rx(&nodes[0], own, sizeof(own));
    CHECK(nodes[0].stats.frames_dropped == dropped + 1);
}

static void test_store_eviction(void)
{
    sim_init(1, 0);
    char t[32];
    for (int i = 0; i < CM_STORE_MAX + 10; i++) {
        snprintf(t, sizeof(t), "n%d", i);
        post(0, t);
    }
    CHECK(nodes[0].count == CM_STORE_MAX);
    uint64_t fp = 0, prev = 0;
    for (int i = 0; i < nodes[0].count; i++) {
        uint64_t k = 0;
        for (int b = 7; b >= 0; b--) k = k << 8 | nodes[0].store[i].short_id[b];
        if (i) CHECK(k > prev); /* sorted ascending, no duplicates */
        prev = k;
        fp += k;
        CHECK(strcmp(nodes[0].store[i].text, "n0") != 0); /* oldest arrivals evicted */
    }
    CHECK(fp == nodes[0].fingerprint);
    CHECK(!cm_node_post(&nodes[0], 0, "n137", 4)); /* identical note already present */
    CHECK(!cm_node_post(&nodes[0], 0, "", 0));
}

int main(void)
{
    test_sha256();
    test_codec();
    test_garbage_rejected();
    test_store_eviction();
    test_two_boxes_one_note();
    test_three_hops();
    test_bulk_lossy();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all core tests passed\n");
    return 0;
}
