/* Captive web portal: one page, a JSON list of notes, and a post endpoint.
 * Every unknown URL redirects to "/", which is what makes phones open the
 * portal automatically after joining the hotspot. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "portal.h"

static const char *TAG = "cm_portal";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static esp_err_t get_index(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    /* EMBED_TXTFILES appends a NUL; don't send it. */
    return httpd_resp_send(req, index_html_start, index_html_end - index_html_start - 1);
}

/* Append s as a JSON string body (without quotes) into out; returns new length. */
static size_t json_escape(char *out, size_t cap, size_t len, const char *s)
{
    for (; *s && len + 7 < cap; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            out[len++] = '\\';
            out[len++] = (char)c;
        } else if (c < 0x20) {
            len += (size_t)snprintf(out + len, cap - len, "\\u%04x", c);
        } else {
            out[len++] = (char)c;
        }
    }
    return len;
}

static esp_err_t get_notes(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    char buf[CM_TEXT_MAX * 6 + 160];
    cm_box_lock();
    cm_node *n = cm_box_node();
    int neighbors = 0;
    for (int i = 0; i < CM_NEIGHBORS_MAX; i++) neighbors += n->neighbors[i].id != 0;
    int count = n->count;
    int len = snprintf(buf, sizeof(buf), "{\"node\":\"%08" PRIx32 "\",\"neighbors\":%d,\"notes\":[", n->id, neighbors);
    cm_box_unlock();
    httpd_resp_send_chunk(req, buf, len);

    /* Copy one note at a time so the mesh task is never blocked on the network. */
    for (int i = 0; i < count; i++) {
        cm_note note;
        cm_box_lock();
        bool ok = i < n->count;
        if (ok) note = n->store[i];
        cm_box_unlock();
        if (!ok) break;

        size_t l = (size_t)snprintf(buf, sizeof(buf),
                                    "%s{\"id\":\"%02x%02x%02x%02x%02x%02x%02x%02x\",\"author\":\"%08" PRIx32
                                    "\",\"t\":%llu,\"seq\":%" PRIu32 ",\"text\":\"",
                                    i ? "," : "", note.short_id[0], note.short_id[1], note.short_id[2],
                                    note.short_id[3], note.short_id[4], note.short_id[5], note.short_id[6],
                                    note.short_id[7], note.author, (unsigned long long)note.time_ms, note.seq);
        l = json_escape(buf, sizeof(buf), l, note.text);
        buf[l++] = '"';
        buf[l++] = '}';
        httpd_resp_send_chunk(req, buf, (ssize_t)l);
    }
    httpd_resp_send_chunk(req, "]}", 2);
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t post_note(httpd_req_t *req)
{
    if (req->content_len == 0 || req->content_len > CM_TEXT_MAX) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "note must be 1-200 bytes");
        return ESP_OK;
    }
    char text[CM_TEXT_MAX];
    size_t got = 0;
    while (got < req->content_len) {
        int r = httpd_req_recv(req, text + got, req->content_len - got);
        if (r <= 0) {
            if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        got += (size_t)r;
    }

    /* The phone supplies the wall-clock time; the box has no RTC. */
    uint64_t t = 0;
    char q[64], v[24];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK && httpd_query_key_value(q, "t", v, sizeof(v)) == ESP_OK)
        t = strtoull(v, NULL, 10);

    cm_box_lock();
    bool ok = cm_node_post(cm_box_node(), t, text, got);
    cm_box_unlock();
    if (!ok) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "rejected (duplicate?)");
        return ESP_OK;
    }
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t redirect(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

void portal_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.lru_purge_enable = true; /* phones hold idle sockets open */
    cfg.stack_size = 6144;       /* get_notes keeps a ~1.4 KB buffer on the stack */
    httpd_handle_t srv;
    if (httpd_start(&srv, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "http server failed to start");
        return;
    }
    const httpd_uri_t uris[] = {
        {.uri = "/", .method = HTTP_GET, .handler = get_index},
        {.uri = "/api/notes", .method = HTTP_GET, .handler = get_notes},
        {.uri = "/api/notes", .method = HTTP_POST, .handler = post_note},
        {.uri = "/*", .method = HTTP_GET, .handler = redirect}, /* must stay last */
    };
    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) httpd_register_uri_handler(srv, &uris[i]);
    dns_start();
    ESP_LOGI(TAG, "portal up");
}
