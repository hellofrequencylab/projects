/*
 * CyberMesh floor v0 core. Portable C99: no OS, no allocation, no vendor calls.
 * Wire format and behavior: docs/spec-floor-v0.md (normative).
 *
 * Not thread-safe. The caller serializes every cm_node_* call on one node.
 */
#ifndef CM_H
#define CM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CM_FRAME_MAX 250
#define CM_TEXT_MAX 200
#define CM_OBJ_HDR 16
#define CM_OBJ_MAX (CM_OBJ_HDR + CM_TEXT_MAX)
#define CM_SHORT_ID 8
#define CM_STORE_MAX 128
#define CM_NEIGHBORS_MAX 8
#define CM_OBJQ_MAX 32

#define CM_BEACON_MS 5000u
#define CM_BEACON_JITTER_MS 1000u
#define CM_INV_REQ_HOLDOFF_MS 3000u
#define CM_TICK_MS 50u
#define CM_OBJ_PER_TICK 2
#define CM_PAGE_BASE_MS 300u     /* wait before next INV page ... */
#define CM_PAGE_PER_ID_MS 25u    /* ... plus this per requested ID */

typedef struct {
    uint32_t author;
    uint64_t time_ms;
    uint8_t text_len;
    char text[CM_TEXT_MAX + 1]; /* NUL-terminated copy for convenience */
    uint8_t short_id[CM_SHORT_ID];
    uint32_t seq; /* local arrival order, not on the wire */
} cm_note;

typedef struct {
    uint32_t id;
    uint32_t last_seen_ms;
    uint32_t last_inv_req_ms;
    bool inv_req_sent;
    bool page_pending;    /* next inventory page to request once GETs have drained */
    uint16_t page_start;
    uint32_t page_due_ms;
    uint16_t count;
    uint64_t fingerprint;
} cm_neighbor;

typedef struct {
    /* Broadcast one frame. Return false if the link is busy; the core does not retry. */
    bool (*send)(void *ctx, const uint8_t *frame, size_t len);
    /* Monotonic milliseconds (wraps at 2^32). */
    uint32_t (*now_ms)(void *ctx);
    /* Uniform random 32-bit value; used for beacon jitter only. */
    uint32_t (*random)(void *ctx);
    /* Optional: called once for every newly stored note (local or received). */
    void (*on_new_note)(void *ctx, const cm_note *note, bool local);
    void *ctx;
} cm_platform;

typedef struct {
    uint32_t frames_rx, frames_tx, frames_dropped, send_busy;
    uint32_t notes_received, notes_posted;
} cm_stats;

typedef struct {
    uint32_t id;
    cm_platform p;
    cm_note store[CM_STORE_MAX]; /* sorted ascending by short ID as u64 LE */
    uint16_t count;
    uint32_t next_seq;
    uint64_t fingerprint;
    cm_neighbor neighbors[CM_NEIGHBORS_MAX];
    uint8_t objq[CM_OBJQ_MAX][CM_SHORT_ID];
    uint8_t objq_len;
    uint32_t next_beacon_ms;
    cm_stats stats;
} cm_node;

void cm_node_init(cm_node *n, uint32_t id, const cm_platform *p);

/* Call at least every CM_TICK_MS. Sends beacons and paces queued objects. */
void cm_node_tick(cm_node *n);

/* Feed every received link frame. */
void cm_node_rx(cm_node *n, const uint8_t *frame, size_t len);

/* Post a note authored on this node. Returns false if the text is empty or too long. */
bool cm_node_post(cm_node *n, uint64_t time_ms, const char *text, size_t len);

/* Serialize note i (0 <= i < count) into the object wire format. Returns length. */
size_t cm_note_encode(const cm_note *note, uint8_t out[CM_OBJ_MAX]);

/* Parse and validate an object. Fills short_id. Returns false if invalid. */
bool cm_note_decode(const uint8_t *buf, size_t len, cm_note *out);

/* Insert an already-validated note (e.g. restored from flash) without callbacks. */
bool cm_node_restore(cm_node *n, const cm_note *note);

/* SHA-256, exposed for tests. */
void cm_sha256(const uint8_t *data, size_t len, uint8_t out[32]);

#ifdef __cplusplus
}
#endif

#endif
