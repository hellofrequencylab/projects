/* CyberMesh floor v0: store, frames and sync loop. See docs/spec-floor-v0.md. */
#include "cm.h"

#include <string.h>

enum {
    T_BEACON = 0x01,
    T_INV_REQ = 0x02,
    T_INV = 0x03,
    T_GET = 0x04,
    T_OBJ = 0x05,
};

#define HDR 8
#define INV_MAX_IDS 28
#define GET_MAX_IDS 29
#define CHANGE_BEACON_MS 200u

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void put64(uint8_t *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t get64(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = v << 8 | p[i];
    return v;
}

/* True once `now` has reached `deadline` (wrap-safe). */
static bool reached(uint32_t now, uint32_t deadline) { return (int32_t)(now - deadline) >= 0; }

/* ---- objects ---- */

size_t cm_note_encode(const cm_note *note, uint8_t out[CM_OBJ_MAX])
{
    out[0] = 0x00; /* obj_version */
    out[1] = 0x00; /* suite: unsigned, SHA-256 */
    out[2] = 0x01; /* type: note */
    put32(out + 3, note->author);
    put64(out + 7, note->time_ms);
    out[15] = note->text_len;
    memcpy(out + CM_OBJ_HDR, note->text, note->text_len);
    return CM_OBJ_HDR + (size_t)note->text_len;
}

bool cm_note_decode(const uint8_t *buf, size_t len, cm_note *out)
{
    if (len < CM_OBJ_HDR + 1 || len > CM_OBJ_MAX) return false;
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01) return false;
    uint8_t tl = buf[15];
    if (tl == 0 || tl > CM_TEXT_MAX || len != CM_OBJ_HDR + (size_t)tl) return false;

    memset(out, 0, sizeof(*out));
    out->author = get32(buf + 3);
    out->time_ms = get64(buf + 7);
    out->text_len = tl;
    memcpy(out->text, buf + CM_OBJ_HDR, tl);
    out->text[tl] = '\0';
    uint8_t digest[32];
    cm_sha256(buf, len, digest);
    memcpy(out->short_id, digest, CM_SHORT_ID);
    return true;
}

/* ---- store (sorted by short ID as u64 LE) ---- */

static int find_pos(const cm_node *n, uint64_t key, bool *found)
{
    int lo = 0, hi = n->count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        uint64_t k = get64(n->store[mid].short_id);
        if (k < key) lo = mid + 1;
        else hi = mid;
    }
    *found = lo < n->count && get64(n->store[lo].short_id) == key;
    return lo;
}

static const cm_note *store_get(const cm_node *n, const uint8_t sid[CM_SHORT_ID])
{
    bool found;
    int pos = find_pos(n, get64(sid), &found);
    return found ? &n->store[pos] : NULL;
}

static void store_remove(cm_node *n, int pos)
{
    n->fingerprint -= get64(n->store[pos].short_id);
    memmove(&n->store[pos], &n->store[pos + 1], (size_t)(n->count - pos - 1) * sizeof(cm_note));
    n->count--;
}

/* Returns the stored copy, or NULL if the note was already present. */
static const cm_note *store_insert(cm_node *n, const cm_note *note)
{
    uint64_t key = get64(note->short_id);
    bool found;
    find_pos(n, key, &found);
    if (found) return NULL;

    if (n->count == CM_STORE_MAX) {
        int oldest = 0;
        for (int i = 1; i < n->count; i++)
            if (n->store[i].seq < n->store[oldest].seq) oldest = i;
        store_remove(n, oldest);
    }
    int pos = find_pos(n, key, &found);
    memmove(&n->store[pos + 1], &n->store[pos], (size_t)(n->count - pos) * sizeof(cm_note));
    n->store[pos] = *note;
    n->store[pos].seq = n->next_seq++;
    n->count++;
    n->fingerprint += key;

    /* Advertise the change soon, so multi-hop sync doesn't wait a full beacon period. */
    uint32_t soon = n->p.now_ms(n->p.ctx) + CHANGE_BEACON_MS;
    if (reached(n->next_beacon_ms, soon)) n->next_beacon_ms = soon;
    return &n->store[pos];
}

bool cm_node_restore(cm_node *n, const cm_note *note) { return store_insert(n, note) != NULL; }

/* ---- frames ---- */

static void hdr(const cm_node *n, uint8_t *f, uint8_t type)
{
    f[0] = 0x43;
    f[1] = 0x4D;
    f[2] = 0x00;
    f[3] = type;
    put32(f + 4, n->id);
}

static bool send(cm_node *n, const uint8_t *f, size_t len)
{
    if (!n->p.send(n->p.ctx, f, len)) {
        n->stats.send_busy++;
        return false;
    }
    n->stats.frames_tx++;
    return true;
}

static void send_beacon(cm_node *n)
{
    uint8_t f[HDR + 10];
    hdr(n, f, T_BEACON);
    put16(f + 8, n->count);
    put64(f + 10, n->fingerprint);
    send(n, f, sizeof(f));
}

static void send_inv_req(cm_node *n, uint32_t dest, uint16_t start)
{
    uint8_t f[HDR + 6];
    hdr(n, f, T_INV_REQ);
    put32(f + 8, dest);
    put16(f + 12, start);
    send(n, f, sizeof(f));
}

static void send_inv(cm_node *n, uint32_t dest, uint16_t start)
{
    uint8_t f[CM_FRAME_MAX];
    hdr(n, f, T_INV);
    put32(f + 8, dest);
    put16(f + 12, start);
    put16(f + 14, n->count);
    uint8_t k = 0;
    for (int i = start; i < n->count && k < INV_MAX_IDS; i++, k++)
        memcpy(f + 17 + CM_SHORT_ID * k, n->store[i].short_id, CM_SHORT_ID);
    f[16] = k;
    send(n, f, 17 + (size_t)CM_SHORT_ID * k);
}

static void objq_push(cm_node *n, const uint8_t sid[CM_SHORT_ID])
{
    for (int i = 0; i < n->objq_len; i++)
        if (memcmp(n->objq[i], sid, CM_SHORT_ID) == 0) return;
    if (n->objq_len == CM_OBJQ_MAX) return;
    memcpy(n->objq[n->objq_len++], sid, CM_SHORT_ID);
}

static void objq_pop(cm_node *n)
{
    memmove(n->objq[0], n->objq[1], (size_t)(n->objq_len - 1) * CM_SHORT_ID);
    n->objq_len--;
}

/* ---- neighbors ---- */

static cm_neighbor *neighbor(cm_node *n, uint32_t id, uint32_t now)
{
    cm_neighbor *slot = &n->neighbors[0];
    for (int i = 0; i < CM_NEIGHBORS_MAX; i++) {
        cm_neighbor *nb = &n->neighbors[i];
        if (nb->id == id) { slot = nb; goto seen; }
        if (nb->id == 0) { slot = nb; break; }
        if ((uint32_t)(now - nb->last_seen_ms) > (uint32_t)(now - slot->last_seen_ms)) slot = nb;
    }
    memset(slot, 0, sizeof(*slot));
    slot->id = id;
seen:
    slot->last_seen_ms = now;
    return slot;
}

/* ---- public API ---- */

void cm_node_init(cm_node *n, uint32_t id, const cm_platform *p)
{
    memset(n, 0, sizeof(*n));
    n->id = id;
    n->p = *p;
    n->next_beacon_ms = p->now_ms(p->ctx) + p->random(p->ctx) % CM_BEACON_JITTER_MS;
}

void cm_node_tick(cm_node *n)
{
    uint32_t now = n->p.now_ms(n->p.ctx);
    if (reached(now, n->next_beacon_ms)) {
        send_beacon(n);
        n->next_beacon_ms = now + CM_BEACON_MS - CM_BEACON_JITTER_MS +
                            n->p.random(n->p.ctx) % (2 * CM_BEACON_JITTER_MS + 1);
    }
    for (int i = 0; i < CM_NEIGHBORS_MAX; i++) {
        cm_neighbor *nb = &n->neighbors[i];
        if (nb->id && nb->page_pending && reached(now, nb->page_due_ms)) {
            nb->page_pending = false;
            send_inv_req(n, nb->id, nb->page_start);
        }
    }
    for (int sent = 0; sent < CM_OBJ_PER_TICK && n->objq_len > 0;) {
        const cm_note *note = store_get(n, n->objq[0]);
        if (note) {
            uint8_t f[HDR + CM_OBJ_MAX];
            hdr(n, f, T_OBJ);
            size_t len = HDR + cm_note_encode(note, f + HDR);
            if (!send(n, f, len)) break; /* link busy: keep it queued for the next tick */
            sent++;
        }
        objq_pop(n);
    }
}

void cm_node_rx(cm_node *n, const uint8_t *f, size_t len)
{
    if (len < HDR || len > CM_FRAME_MAX || f[0] != 0x43 || f[1] != 0x4D || f[2] != 0x00) goto drop;
    uint32_t src = get32(f + 4);
    if (src == n->id || src == 0) goto drop;
    uint32_t now = n->p.now_ms(n->p.ctx);

    switch (f[3]) {
    case T_BEACON: {
        if (len != HDR + 10) goto drop;
        cm_neighbor *nb = neighbor(n, src, now);
        nb->count = get16(f + 8);
        nb->fingerprint = get64(f + 10);
        bool differs = nb->count != n->count || nb->fingerprint != n->fingerprint;
        bool held_off = nb->inv_req_sent && !reached(now, nb->last_inv_req_ms + CM_INV_REQ_HOLDOFF_MS);
        if (nb->count > 0 && differs && !held_off) {
            send_inv_req(n, src, 0);
            nb->inv_req_sent = true;
            nb->last_inv_req_ms = now;
        }
        break;
    }
    case T_INV_REQ:
        if (len != HDR + 6) goto drop;
        neighbor(n, src, now);
        if (get32(f + 8) == n->id) send_inv(n, src, get16(f + 12));
        break;
    case T_INV: {
        if (len < 17 || f[16] > INV_MAX_IDS || len != 17 + (size_t)CM_SHORT_ID * f[16]) goto drop;
        neighbor(n, src, now);
        uint32_t dest = get32(f + 8);
        uint16_t start = get16(f + 12), total = get16(f + 14);
        uint8_t k = f[16];

        uint8_t g[CM_FRAME_MAX];
        hdr(n, g, T_GET);
        put32(g + 8, src);
        uint8_t want = 0;
        for (int i = 0; i < k && want < GET_MAX_IDS; i++) {
            const uint8_t *sid = f + 17 + CM_SHORT_ID * i;
            if (!store_get(n, sid)) memcpy(g + 13 + CM_SHORT_ID * want++, sid, CM_SHORT_ID);
        }
        g[12] = want;
        if (want) send(n, g, 13 + (size_t)CM_SHORT_ID * want);
        if (dest == n->id && (uint32_t)start + k < total) {
            /* Pace paging so the sender's object queue isn't overrun. */
            cm_neighbor *nb = neighbor(n, src, now);
            nb->page_pending = true;
            nb->page_start = (uint16_t)(start + k);
            nb->page_due_ms = want ? now + CM_PAGE_BASE_MS + CM_PAGE_PER_ID_MS * want : now;
        }
        break;
    }
    case T_GET: {
        if (len < 13 || f[12] > GET_MAX_IDS || len != 13 + (size_t)CM_SHORT_ID * f[12]) goto drop;
        neighbor(n, src, now);
        if (get32(f + 8) != n->id) break;
        for (int i = 0; i < f[12]; i++) {
            const uint8_t *sid = f + 13 + CM_SHORT_ID * i;
            if (store_get(n, sid)) objq_push(n, sid);
        }
        break;
    }
    case T_OBJ: {
        cm_note note;
        if (!cm_note_decode(f + HDR, len - HDR, &note)) goto drop;
        neighbor(n, src, now);
        const cm_note *stored = store_insert(n, &note);
        if (stored) {
            n->stats.notes_received++;
            if (n->p.on_new_note) n->p.on_new_note(n->p.ctx, stored, false);
        }
        break;
    }
    default:
        goto drop;
    }
    n->stats.frames_rx++;
    return;
drop:
    n->stats.frames_dropped++;
}

bool cm_node_post(cm_node *n, uint64_t time_ms, const char *text, size_t len)
{
    if (len == 0 || len > CM_TEXT_MAX) return false;
    cm_note note = {0};
    note.author = n->id;
    note.time_ms = time_ms;
    note.text_len = (uint8_t)len;
    memcpy(note.text, text, len);

    uint8_t buf[CM_OBJ_MAX];
    size_t olen = cm_note_encode(&note, buf);
    if (!cm_note_decode(buf, olen, &note)) return false; /* fills short_id */
    const cm_note *stored = store_insert(n, &note);
    if (!stored) return false; /* identical note already present */
    n->stats.notes_posted++;
    if (n->p.on_new_note) n->p.on_new_note(n->p.ctx, stored, true);
    return true;
}
