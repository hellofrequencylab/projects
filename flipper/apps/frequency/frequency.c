// Frequency: the Flipper as a CyberUnicorn console tag writer.
//
// The console at /admin/cyberunicorn talks to a device over Web Serial with one line per message
// (frequency-web lib/device/protocol.ts, cyberpunkunicorn-os apps/remote/PROTOCOL.md):
//
//   host   -> device   rpc {"id":<int>,"cmd":"<name>","args":{...}}\n
//   device -> host     @rpc {"id":<same>,"ok":true,"data":{...}}
//                      @rpc {"id":<same>,"ok":false,"error":"<code>","message":"<text>"}
//                      @evt {"event":"<name>","data":{...}}
//
// While this app is open the Flipper shows a second USB serial port (CDC channel 1); the CLI keeps
// channel 0. The app answers the commands the console needs to write a placard or member card:
// system.status, system.caps, hat.probe, frequency.write_tag and frequency.read_tag. Everything else
// gets `unknown_command`, which the console already handles.

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_usb_cdc.h>
#include <cli/cli_vcp.h>
#include <gui/gui.h>
#include <input/input.h>
#include <nfc/nfc.h>
#include <nfc/nfc_poller.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <nfc/protocols/mf_ultralight/mf_ultralight_poller.h>

#include <stdio.h>
#include <string.h>

#define TAG "Frequency"

#define APP_VERSION "0.1.0"
#define VCP_CH      1
#define LINE_MAX    600
#define OUT_MAX     700
#define URL_MAX     256

#define EVT_RX       (1UL << 0)
#define EVT_EXIT     (1UL << 1)
#define EVT_NFC_DONE (1UL << 2)
#define EVT_ALL      (EVT_RX | EVT_EXIT | EVT_NFC_DONE)

#define TAP_DEFAULT_MS 30000
#define TAP_MAX_MS     60000

typedef enum {
    TagOk,
    TagNotNtag,
    TagLocked,
    TagTooLong,
    TagWriteFailed,
    TagCardError,
    TagNoNdef,
} TagResult;

typedef struct {
    // Request (set before the poller starts).
    bool write;
    uint8_t ndef[URL_MAX + 16];
    size_t ndef_len;
    // Result (set by the NFC worker thread, read after EVT_NFC_DONE).
    TagResult result;
    MfUltralightType type;
    uint8_t uid[7];
    uint16_t capacity;
    char uri[URL_MAX + 32];
    FuriThreadId waiter;
} TagJob;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    CliVcp* cli_vcp;
    FuriThreadId thread;
    FuriSemaphore* tx_sem;
    FuriMutex* ui_mutex;
    bool usb_ok;
    char status[48];
    char detail[48];
    char line[LINE_MAX];
    size_t line_len;
    bool line_overflow;
    TagJob job;
} FrequencyApp;

// ── UI ──────────────────────────────────────────────────────────────────────────────────────────

/* Runs on the GUI thread. No allocation, no blocking, no I/O. */
static void frequency_draw_callback(Canvas* canvas, void* context) {
    FrequencyApp* app = context;
    if(furi_mutex_acquire(app->ui_mutex, 25) != FuriStatusOk) return;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "Frequency");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 28, app->status);
    canvas_draw_str(canvas, 2, 40, app->detail);
    canvas_draw_str(canvas, 2, 62, "Back to exit");
    furi_mutex_release(app->ui_mutex);
}

static void frequency_input_callback(InputEvent* event, void* context) {
    FrequencyApp* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyBack) {
        furi_thread_flags_set(app->thread, EVT_EXIT);
    }
}

static void frequency_set_status(FrequencyApp* app, const char* status, const char* detail) {
    furi_mutex_acquire(app->ui_mutex, FuriWaitForever);
    strlcpy(app->status, status, sizeof(app->status));
    strlcpy(app->detail, detail ? detail : "", sizeof(app->detail));
    furi_mutex_release(app->ui_mutex);
    view_port_update(app->view_port);
}

// ── USB serial (CDC channel 1) ──────────────────────────────────────────────────────────────────

static void vcp_on_tx_complete(void* context) {
    FrequencyApp* app = context;
    furi_semaphore_release(app->tx_sem);
}

static void vcp_on_rx(void* context) {
    FrequencyApp* app = context;
    furi_thread_flags_set(app->thread, EVT_RX);
}

static const CdcCallbacks cdc_callbacks = {
    .tx_ep_callback = vcp_on_tx_complete,
    .rx_ep_callback = vcp_on_rx,
    .state_callback = NULL,
    .ctrl_line_callback = NULL,
    .config_callback = NULL,
};

static void vcp_send(FrequencyApp* app, const char* text, size_t len) {
    while(len > 0) {
        size_t chunk = len > CDC_DATA_SZ ? CDC_DATA_SZ : len;
        // No host reading: drop the rest instead of blocking the app.
        if(furi_semaphore_acquire(app->tx_sem, 200) != FuriStatusOk) return;
        furi_hal_cdc_send(VCP_CH, (uint8_t*)text, chunk);
        text += chunk;
        len -= chunk;
    }
}

/* Every protocol line goes out as "\n<line>\n" (the host drops blank lines), which also keeps a
 * line from ending exactly on a 64-byte packet boundary with nothing after it. */
static void vcp_send_line(FrequencyApp* app, const char* line) {
    vcp_send(app, "\n", 1);
    vcp_send(app, line, strlen(line));
    vcp_send(app, "\n", 1);
}

// ── Tiny JSON helpers (the request shape is fixed and small) ────────────────────────────────────

/* Find `"key":` in json and return a pointer just past the colon and any spaces, or NULL. */
static const char* json_find(const char* json, const char* key) {
    size_t klen = strlen(key);
    for(const char* p = json; (p = strchr(p, '"')) != NULL; p++) {
        if(strncmp(p + 1, key, klen) == 0 && p[klen + 1] == '"') {
            const char* q = p + klen + 2;
            while(*q == ' ')
                q++;
            if(*q != ':') continue;
            q++;
            while(*q == ' ')
                q++;
            return q;
        }
    }
    return NULL;
}

static bool json_int(const char* json, const char* key, int32_t* out) {
    const char* v = json_find(json, key);
    if(!v || (*v != '-' && (*v < '0' || *v > '9'))) return false;
    *out = (int32_t)strtol(v, NULL, 10);
    return true;
}

/* Copy a JSON string value, decoding \" \\ \/ and ASCII \u00XX. False if absent, not a string or
 * longer than out_size - 1. */
static bool json_str(const char* json, const char* key, char* out, size_t out_size) {
    const char* v = json_find(json, key);
    if(!v || *v != '"') return false;
    v++;
    size_t n = 0;
    while(*v && *v != '"') {
        char c = *v++;
        if(c == '\\') {
            char e = *v++;
            if(e == 'u') {
                unsigned code = 0;
                if(sscanf(v, "%4x", &code) != 1 || code > 0x7F) return false;
                c = (char)code;
                v += 4;
            } else if(e == 'n') {
                c = '\n';
            } else if(e == 't') {
                c = '\t';
            } else if(e == '"' || e == '\\' || e == '/') {
                c = e;
            } else {
                return false;
            }
        }
        if(n + 1 >= out_size) return false;
        out[n++] = c;
    }
    if(*v != '"') return false;
    out[n] = '\0';
    return true;
}

/* Append src to dst as a JSON string body (no quotes). */
static void json_escape(char* dst, size_t dst_size, const char* src) {
    size_t n = strlen(dst);
    for(; *src && n + 7 < dst_size; src++) {
        unsigned char c = (unsigned char)*src;
        if(c == '"' || c == '\\') {
            dst[n++] = '\\';
            dst[n++] = (char)c;
        } else if(c < 0x20) {
            n += snprintf(dst + n, dst_size - n, "\\u%04x", c);
        } else {
            dst[n++] = (char)c;
        }
    }
    dst[n] = '\0';
}

static void reply_ok(FrequencyApp* app, int32_t id, const char* data) {
    char out[OUT_MAX];
    snprintf(out, sizeof(out), "@rpc {\"id\":%ld,\"ok\":true,\"data\":%s}", (long)id, data);
    vcp_send_line(app, out);
}

static void reply_err(FrequencyApp* app, int32_t id, const char* code, const char* message) {
    char msg[160] = "";
    json_escape(msg, sizeof(msg), message);
    char out[OUT_MAX];
    snprintf(
        out,
        sizeof(out),
        "@rpc {\"id\":%ld,\"ok\":false,\"error\":\"%s\",\"message\":\"%s\"}",
        (long)id,
        code,
        msg);
    vcp_send_line(app, out);
}

// ── NTAG ────────────────────────────────────────────────────────────────────────────────────────

static const char* ntag_name(MfUltralightType type) {
    switch(type) {
    case MfUltralightTypeNTAG213:
        return "NTAG213";
    case MfUltralightTypeNTAG215:
        return "NTAG215";
    case MfUltralightTypeNTAG216:
        return "NTAG216";
    default:
        return "Ultralight";
    }
}

static bool is_ntag(MfUltralightType type) {
    return type == MfUltralightTypeNTAG213 || type == MfUltralightTypeNTAG215 ||
           type == MfUltralightTypeNTAG216;
}

/* NFC Forum URI prefixes the encoder and decoder share. */
static const char* const uri_prefixes[] =
    {"", "http://www.", "https://www.", "http://", "https://"};

/* One NDEF message with one short URI record, wrapped in a TLV and terminated. */
static size_t ndef_build(const char* url, uint8_t* out, size_t out_size) {
    uint8_t code = 0;
    size_t skip = 0;
    for(uint8_t i = 1; i < COUNT_OF(uri_prefixes); i++) {
        size_t plen = strlen(uri_prefixes[i]);
        if(plen > skip && strncmp(url, uri_prefixes[i], plen) == 0) {
            code = i;
            skip = plen;
        }
    }
    size_t rest = strlen(url) - skip;
    size_t payload = 1 + rest;
    size_t record = 4 + payload;
    if(payload > 255 || record > 254 || record + 3 > out_size) return 0;
    size_t n = 0;
    out[n++] = 0x03; // NDEF message TLV
    out[n++] = (uint8_t)record;
    out[n++] = 0xD1; // MB | ME | SR, TNF well-known
    out[n++] = 0x01; // type length
    out[n++] = (uint8_t)payload;
    out[n++] = 'U';
    out[n++] = code;
    memcpy(out + n, url + skip, rest);
    n += rest;
    out[n++] = 0xFE; // terminator TLV
    return n;
}

/* Pull the first URI record out of user memory. */
static bool ndef_parse(const uint8_t* mem, size_t len, char* uri, size_t uri_size) {
    size_t i = 0;
    while(i < len) {
        uint8_t t = mem[i];
        if(t == 0x00) {
            i++;
            continue;
        }
        if(t == 0xFE || i + 1 >= len) return false;
        size_t l = mem[i + 1];
        size_t v = i + 2;
        if(l == 0xFF) {
            if(i + 3 >= len) return false;
            l = ((size_t)mem[i + 2] << 8) | mem[i + 3];
            v = i + 4;
        }
        if(t == 0x03) {
            if(v + 4 > len) return false;
            uint8_t hdr = mem[v];
            uint8_t type_len = mem[v + 1];
            bool sr = hdr & 0x10;
            bool il = hdr & 0x08;
            size_t p = v + 2;
            size_t payload_len;
            if(sr) {
                payload_len = mem[p++];
            } else {
                if(p + 4 > len) return false;
                payload_len = ((size_t)mem[p] << 24) | ((size_t)mem[p + 1] << 16) |
                              ((size_t)mem[p + 2] << 8) | mem[p + 3];
                p += 4;
            }
            if(il) p += 1 + mem[p];
            if((hdr & 0x07) != 0x01 || type_len != 1 || p >= len || mem[p] != 'U') return false;
            p += type_len;
            if(payload_len < 1 || p + payload_len > len) return false;
            uint8_t code = mem[p];
            const char* prefix = code < COUNT_OF(uri_prefixes) ? uri_prefixes[code] : "";
            size_t plen = strlen(prefix);
            size_t rest = payload_len - 1;
            if(plen + rest + 1 > uri_size) return false;
            memcpy(uri, prefix, plen);
            memcpy(uri + plen, mem + p + 1, rest);
            uri[plen + rest] = '\0';
            return true;
        }
        i = v + l;
    }
    return false;
}

static bool read4(MfUltralightPoller* poller, uint8_t page, uint8_t out[16]) {
    MfUltralightPageReadCommandData data;
    if(mf_ultralight_poller_read_page(poller, page, &data) != MfUltralightErrorNone) return false;
    memcpy(out, data.page, 16);
    return true;
}

/* The whole tap, on the NFC worker thread: identify, check, write, read back. */
static TagResult tag_session(MfUltralightPoller* poller, TagJob* job) {
    MfUltralightVersion version;
    if(mf_ultralight_poller_read_version(poller, &version) != MfUltralightErrorNone) {
        return TagNotNtag;
    }
    job->type = mf_ultralight_get_type_by_version(&version);
    if(!is_ntag(job->type)) return TagNotNtag;

    uint16_t cfg = mf_ultralight_get_config_page_num(job->type);
    uint16_t first_user = 4;
    uint16_t end_user = cfg - 1; // the dynamic lock page
    job->capacity = (end_user - first_user) * 4;

    uint8_t head[16];
    if(!read4(poller, 0, head)) return TagCardError;
    job->uid[0] = head[0];
    job->uid[1] = head[1];
    job->uid[2] = head[2];
    memcpy(job->uid + 3, head + 4, 4);

    if(job->write) {
        // Pages cfg-1 .. cfg+2: dynamic lock, CFG0 (AUTH0 in byte 3), CFG1, PWD.
        uint8_t tail[16];
        if(!read4(poller, end_user, tail)) return TagCardError;
        bool static_locked = head[10] != 0 || head[11] != 0;
        bool dynamic_locked = tail[0] != 0 || tail[1] != 0;
        bool password = tail[7] <= end_user;
        if(static_locked || dynamic_locked || password) return TagLocked;
        if(job->ndef_len > job->capacity) return TagTooLong;

        // A factory-blank CC (page 3) is zero; it is OTP, so only fill it in when empty.
        if(head[12] == 0 && head[13] == 0 && head[14] == 0 && head[15] == 0) {
            uint8_t size = job->type == MfUltralightTypeNTAG213 ? 0x12 :
                           job->type == MfUltralightTypeNTAG215 ? 0x3E :
                                                                  0x6D;
            MfUltralightPage cc = {.data = {0xE1, 0x10, size, 0x00}};
            if(mf_ultralight_poller_write_page(poller, 3, &cc) != MfUltralightErrorNone) {
                return TagWriteFailed;
            }
        }

        uint16_t pages = (job->ndef_len + 3) / 4;
        for(uint16_t i = 0; i < pages; i++) {
            MfUltralightPage page = {0};
            size_t off = (size_t)i * 4;
            size_t n = job->ndef_len - off < 4 ? job->ndef_len - off : 4;
            memcpy(page.data, job->ndef + off, n);
            if(mf_ultralight_poller_write_page(poller, first_user + i, &page) !=
               MfUltralightErrorNone) {
                return TagWriteFailed;
            }
        }

        // Read back what was written.
        uint8_t back[URL_MAX + 32];
        size_t got = 0;
        for(uint16_t p = first_user; got < job->ndef_len; p += 4) {
            if(!read4(poller, p, back + got)) return TagWriteFailed;
            got += 16;
        }
        if(memcmp(back, job->ndef, job->ndef_len) != 0) return TagWriteFailed;
        if(!ndef_parse(back, got, job->uri, sizeof(job->uri))) return TagWriteFailed;
        return TagOk;
    }

    // Read: user memory until the NDEF TLV is complete (a placard link fits in a few reads).
    uint8_t mem[URL_MAX + 32];
    size_t got = 0;
    for(uint16_t p = first_user; p < end_user && got + 16 <= sizeof(mem); p += 4) {
        if(!read4(poller, p, mem + got)) return TagCardError;
        got += 16;
        if(ndef_parse(mem, got, job->uri, sizeof(job->uri))) return TagOk;
        if(memchr(mem, 0xFE, got)) break;
    }
    return TagNoNdef;
}

static NfcCommand tag_poller_callback(NfcGenericEventEx event, void* context) {
    TagJob* job = context;
    Iso14443_3aPollerEvent* iso_event = event.parent_event_data;
    // No card yet: the 3A poller reports an error and polls again.
    if(iso_event->type != Iso14443_3aPollerEventTypeReady) return NfcCommandContinue;
    job->result = tag_session((MfUltralightPoller*)event.poller, job);
    furi_thread_flags_set(job->waiter, EVT_NFC_DONE);
    return NfcCommandStop;
}

/* Run one tap. Returns false on timeout or when Back is pressed (exit set). */
static bool tag_run(FrequencyApp* app, uint32_t timeout_ms, bool* exit) {
    app->job.waiter = furi_thread_get_current_id();
    furi_thread_flags_clear(EVT_NFC_DONE);
    Nfc* nfc = nfc_alloc();
    NfcPoller* poller = nfc_poller_alloc(nfc, NfcProtocolMfUltralight);
    nfc_poller_start_ex(poller, tag_poller_callback, &app->job);

    bool done = false;
    uint32_t deadline = furi_get_tick() + furi_ms_to_ticks(timeout_ms);
    while(!done) {
        int32_t left = (int32_t)(deadline - furi_get_tick());
        if(left <= 0) break;
        uint32_t flags = furi_thread_flags_wait(EVT_NFC_DONE | EVT_EXIT, FuriFlagWaitAny, left);
        if(flags & FuriFlagError) break; // timeout
        if(flags & EVT_NFC_DONE) done = true;
        if(flags & EVT_EXIT) {
            *exit = true;
            break;
        }
    }

    nfc_poller_stop(poller);
    nfc_poller_free(poller);
    nfc_free(nfc);
    return done;
}

static void uid_hex(const uint8_t uid[7], char out[22]) {
    for(size_t i = 0; i < 7; i++) {
        snprintf(out + i * 3, 4, i < 6 ? "%02X:" : "%02X", uid[i]);
    }
}

/* host + path of a URL, without the scheme or query (the console logs this, never the code). */
static void url_written(const char* url, char* out, size_t out_size) {
    const char* p = strstr(url, "://");
    p = p ? p + 3 : url;
    size_t n = strcspn(p, "?#");
    if(n >= out_size) n = out_size - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

static void tag_reply(FrequencyApp* app, int32_t id, bool write, const char* url) {
    TagJob* job = &app->job;
    switch(job->result) {
    case TagOk:
        break;
    case TagNotNtag:
        frequency_set_status(app, "Not an NTAG", "Use an NTAG213/215/216");
        reply_err(app, id, "write_failed", "Not an NTAG213, 215 or 216");
        return;
    case TagLocked:
        frequency_set_status(app, "Tag is locked", "Use a blank tag");
        reply_err(app, id, "write_failed", "Tag is locked or password protected");
        return;
    case TagTooLong:
        frequency_set_status(app, "Tag too small", "Use an NTAG215 or 216");
        reply_err(app, id, "write_failed", "Link too long for this tag");
        return;
    case TagWriteFailed:
        frequency_set_status(app, "Write failed", "Hold the tag still");
        reply_err(app, id, "write_failed", "Write did not verify");
        return;
    case TagCardError:
        frequency_set_status(app, "Card error", "Lift it off and retry");
        reply_err(app, id, "card_error", "Could not talk to the card");
        return;
    case TagNoNdef:
        if(write) break;
        job->uri[0] = '\0';
        break;
    }

    char uid[22];
    uid_hex(job->uid, uid);
    const char* type = ntag_name(job->type);
    char data[OUT_MAX - 64];
    snprintf(
        data,
        sizeof(data),
        "{\"uid\":\"%s\",\"type\":\"%s\",\"ntag\":true,\"locked\":false,\"capacity\":%u,\"uri\":\"",
        uid,
        type,
        job->capacity);
    json_escape(data, sizeof(data), job->uri);
    strlcat(data, "\"", sizeof(data));
    if(write) {
        char written[96];
        url_written(url, written, sizeof(written));
        strlcat(data, ",\"written\":\"", sizeof(data));
        json_escape(data, sizeof(data), written);
        strlcat(data, "\"", sizeof(data));
    }
    strlcat(data, "}", sizeof(data));
    reply_ok(app, id, data);

    char detail[48];
    snprintf(detail, sizeof(detail), "%s %s", type, uid);
    frequency_set_status(app, write ? "Tag written" : "Tag read", detail);
}

// ── Commands ────────────────────────────────────────────────────────────────────────────────────

static bool handle_tag(FrequencyApp* app, int32_t id, const char* json, bool write) {
    int32_t timeout_ms = write ? TAP_DEFAULT_MS : 5000;
    json_int(json, "timeout_ms", &timeout_ms);
    if(timeout_ms < 1000) timeout_ms = 1000;
    if(timeout_ms > TAP_MAX_MS) timeout_ms = TAP_MAX_MS;

    static char url[URL_MAX];
    TagJob* job = &app->job;
    memset(job, 0, sizeof(*job));
    job->write = write;
    if(write) {
        if(!json_str(json, "url", url, sizeof(url)) || strncmp(url, "http", 4) != 0) {
            reply_err(app, id, "bad_args", "url must be an http(s) link");
            return true;
        }
        job->ndef_len = ndef_build(url, job->ndef, sizeof(job->ndef));
        if(job->ndef_len == 0) {
            reply_err(app, id, "bad_args", "url too long");
            return true;
        }
    }

    vcp_send_line(app, "@evt {\"event\":\"frequency.waiting_for_tag\",\"data\":{}}");
    frequency_set_status(
        app, write ? "Tap a tag to write" : "Tap a tag to read", "Hold it on the back");

    bool exit = false;
    if(!tag_run(app, (uint32_t)timeout_ms, &exit)) {
        reply_err(app, id, "timeout", "No card");
        frequency_set_status(app, "No card", "Console connected");
        return !exit;
    }
    tag_reply(app, id, write, url);
    return true;
}

/* Handle one request line. Returns false when the app should exit. */
static bool handle_line(FrequencyApp* app, char* line) {
    while(*line == ' ' || *line == '\r')
        line++;
    if(strncmp(line, "rpc ", 4) != 0) return true; // not ours
    const char* json = line + 4;

    int32_t id = 0;
    char cmd[40];
    if(!json_int(json, "id", &id) || !json_str(json, "cmd", cmd, sizeof(cmd))) {
        reply_err(app, id, "bad_json", "Expected {id, cmd, args}");
        return true;
    }

    char data[OUT_MAX - 64];
    if(strcmp(cmd, "system.status") == 0) {
        snprintf(
            data,
            sizeof(data),
            "{\"os\":\"Flipper Zero\",\"version\":\"" APP_VERSION
            "\",\"device\":\"%s\",\"heap_free\":%u,\"uptime_s\":%lu}",
            furi_hal_version_get_name_ptr() ? furi_hal_version_get_name_ptr() : "",
            (unsigned)memmgr_get_free_heap(),
            (unsigned long)(furi_get_tick() / furi_kernel_get_tick_frequency()));
        reply_ok(app, id, data);
        frequency_set_status(app, "Console connected", "Ready for a tag");
    } else if(strcmp(cmd, "system.caps") == 0) {
        reply_ok(
            app,
            id,
            "{\"proto\":1,\"version\":\"" APP_VERSION
            "\",\"device\":\"flipper\",\"commands\":[\"system.status\",\"system.caps\","
            "\"hat.probe\",\"frequency.read_tag\",\"frequency.write_tag\"]}");
    } else if(strcmp(cmd, "hat.probe") == 0) {
        // The console gates its tag tools on `pn532`: the Flipper's own NFC reader stands in.
        reply_ok(
            app, id, "{\"pn532\":true,\"live_count\":1,\"summary\":\"Flipper Zero NFC reader\"}");
    } else if(strcmp(cmd, "frequency.write_tag") == 0) {
        return handle_tag(app, id, json, true);
    } else if(strcmp(cmd, "frequency.read_tag") == 0) {
        return handle_tag(app, id, json, false);
    } else {
        reply_err(app, id, "unknown_command", "Not on the Flipper");
    }
    return true;
}

static bool frequency_on_rx(FrequencyApp* app) {
    uint8_t buf[CDC_DATA_SZ];
    int32_t len;
    while((len = furi_hal_cdc_receive(VCP_CH, buf, sizeof(buf))) > 0) {
        for(int32_t i = 0; i < len; i++) {
            char c = (char)buf[i];
            if(c == '\n') {
                app->line[app->line_len] = '\0';
                bool overflow = app->line_overflow;
                app->line_len = 0;
                app->line_overflow = false;
                if(overflow) {
                    reply_err(app, 0, "bad_json", "Line too long");
                } else if(!handle_line(app, app->line)) {
                    return false;
                }
            } else if(app->line_len + 1 < LINE_MAX) {
                app->line[app->line_len++] = c;
            } else {
                app->line_overflow = true;
            }
        }
        if(len < CDC_DATA_SZ) break;
    }
    return true;
}

// ── App ─────────────────────────────────────────────────────────────────────────────────────────

int32_t frequency_app(void* p) {
    UNUSED(p);
    FrequencyApp* app = malloc(sizeof(FrequencyApp));
    memset(app, 0, sizeof(FrequencyApp));
    app->thread = furi_thread_get_current_id();
    app->tx_sem = furi_semaphore_alloc(1, 1);
    app->ui_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    strlcpy(app->status, "Starting USB", sizeof(app->status));

    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, frequency_draw_callback, app);
    view_port_input_callback_set(app->view_port, frequency_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    // Second USB serial port for the console; the CLI stays on the first (same as USB-UART Bridge).
    app->cli_vcp = furi_record_open(RECORD_CLI_VCP);
    furi_hal_usb_unlock();
    app->usb_ok = furi_hal_usb_set_config(&usb_cdc_dual, NULL);
    if(app->usb_ok) {
        cli_vcp_enable(app->cli_vcp);
        furi_hal_cdc_set_callbacks(VCP_CH, (CdcCallbacks*)&cdc_callbacks, app);
        frequency_set_status(app, "Waiting for console", "Pick the 2nd Flipper port");
        vcp_send_line(app, "@evt {\"event\":\"hello\",\"data\":{\"os\":\"Flipper Zero\"}}");
    } else {
        frequency_set_status(app, "USB is busy", "Close qFlipper, reopen app");
    }

    bool running = true;
    while(running) {
        uint32_t flags = furi_thread_flags_wait(EVT_ALL, FuriFlagWaitAny, FuriWaitForever);
        if(flags & FuriFlagError) continue;
        if(flags & EVT_EXIT) break;
        if((flags & EVT_RX) && app->usb_ok) running = frequency_on_rx(app);
    }

    if(app->usb_ok) {
        furi_hal_cdc_set_callbacks(VCP_CH, NULL, NULL);
        cli_vcp_disable(app->cli_vcp);
        furi_hal_usb_unlock();
        furi_hal_usb_set_config(&usb_cdc_single, NULL);
        cli_vcp_enable(app->cli_vcp);
    }
    furi_record_close(RECORD_CLI_VCP);

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_mutex_free(app->ui_mutex);
    furi_semaphore_free(app->tx_sem);
    free(app);
    return 0;
}
