/* CyberMesh box firmware, floor v0: SoftAP + captive portal for phones,
 * ESP-NOW broadcast to other boxes, and the portable sync core in ../core. */
#include <string.h>

#include "cm.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "portal.h"

static const char *TAG = "cybermesh";
static const uint8_t BROADCAST[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

typedef struct {
    uint8_t len;
    uint8_t data[CM_FRAME_MAX];
} rx_frame_t;

static cm_node s_node;
static SemaphoreHandle_t s_lock;
static QueueHandle_t s_rxq;

/* ---- platform callbacks for the core ---- */

static bool link_send(void *ctx, const uint8_t *frame, size_t len)
{
    (void)ctx;
    return esp_now_send(BROADCAST, frame, len) == ESP_OK;
}

static uint32_t now_ms(void *ctx)
{
    (void)ctx;
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static uint32_t rand32(void *ctx)
{
    (void)ctx;
    return esp_random();
}

static void on_new_note(void *ctx, const cm_note *note, bool local)
{
    (void)ctx;
    ESP_LOGI(TAG, "%s note from %08" PRIx32 ": %s", local ? "posted" : "received", note->author, note->text);
}

/* Runs in the Wi-Fi task: copy and hand off, never block. */
static void espnow_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    if (len <= 0 || len > CM_FRAME_MAX) return;
    rx_frame_t f = {.len = (uint8_t)len};
    memcpy(f.data, data, (size_t)len);
    xQueueSend(s_rxq, &f, 0);
}

static void mesh_task(void *arg)
{
    (void)arg;
    rx_frame_t f;
    TickType_t last_tick = xTaskGetTickCount();
    for (;;) {
        if (xQueueReceive(s_rxq, &f, pdMS_TO_TICKS(CM_TICK_MS)) == pdTRUE) {
            xSemaphoreTake(s_lock, portMAX_DELAY);
            cm_node_rx(&s_node, f.data, f.len);
            xSemaphoreGive(s_lock);
        }
        if (xTaskGetTickCount() - last_tick >= pdMS_TO_TICKS(CM_TICK_MS)) {
            last_tick = xTaskGetTickCount();
            xSemaphoreTake(s_lock, portMAX_DELAY);
            cm_node_tick(&s_node);
            xSemaphoreGive(s_lock);
        }
    }
}

/* ---- accessors for the portal (portal.h) ---- */

void cm_box_lock(void) { xSemaphoreTake(s_lock, portMAX_DELAY); }
void cm_box_unlock(void) { xSemaphoreGive(s_lock); }
cm_node *cm_box_node(void) { return &s_node; }

/* ---- startup ---- */

static uint32_t node_id_from_mac(void)
{
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    uint32_t id = (uint32_t)mac[2] | (uint32_t)mac[3] << 8 | (uint32_t)mac[4] << 16 | (uint32_t)mac[5] << 24;
    return id ? id : 1; /* 0 is reserved */
}

static void wifi_start(uint32_t id)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap = {0};
    int n = snprintf((char *)ap.ap.ssid, sizeof(ap.ap.ssid), "CyberMesh-%04" PRIX32, id & 0xFFFF);
    ap.ap.ssid_len = (uint8_t)n;
    ap.ap.channel = CONFIG_CM_CHANNEL;
    ap.ap.authmode = WIFI_AUTH_OPEN;
    ap.ap.max_connection = 4;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "hotspot %s on channel %d", (char *)ap.ap.ssid, CONFIG_CM_CHANNEL);
}

static void espnow_start(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv));
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, BROADCAST, ESP_NOW_ETH_ALEN);
    peer.channel = 0; /* the AP's current channel */
    peer.ifidx = WIFI_IF_AP;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init(); /* the Wi-Fi driver needs NVS */
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    s_lock = xSemaphoreCreateMutex();
    s_rxq = xQueueCreate(16, sizeof(rx_frame_t));
    uint32_t id = node_id_from_mac();
    cm_platform p = {
        .send = link_send, .now_ms = now_ms, .random = rand32, .on_new_note = on_new_note, .ctx = NULL};
    cm_node_init(&s_node, id, &p);
    ESP_LOGI(TAG, "node %08" PRIx32 ", floor v0", id);

    wifi_start(id);
    espnow_start();
    xTaskCreate(mesh_task, "cm_mesh", 6144, NULL, 5, NULL);
    portal_start();
}
