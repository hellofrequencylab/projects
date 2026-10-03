/* Captive DNS: answer every A query with the box's own address, so a phone
 * that joins the hotspot is sent to the portal. Other query types get an
 * empty answer instead of a timeout. */
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "portal.h"

static const char *TAG = "cm_dns";

static void dns_task(void *arg)
{
    (void)arg;
    esp_netif_ip_info_t ip;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip));

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(53), .sin_addr.s_addr = htonl(INADDR_ANY)};
    if (sock < 0 || bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "cannot bind :53");
        vTaskDelete(NULL);
        return;
    }

    uint8_t buf[512];
    for (;;) {
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        int len = recvfrom(sock, buf, sizeof(buf) - 16, 0, (struct sockaddr *)&from, &flen);
        if (len < 12) continue;
        if (buf[2] & 0x80) continue;                  /* not a query */
        if (buf[4] != 0 || buf[5] != 1) continue;     /* exactly one question */

        /* Walk the question name to find QTYPE. */
        int p = 12;
        while (p < len && buf[p] != 0) {
            if (buf[p] & 0xC0) { p = len; break; }    /* no compression in questions */
            p += buf[p] + 1;
        }
        if (p + 5 > len) continue;
        int qend = p + 5;                              /* zero byte + QTYPE + QCLASS */
        bool is_a = buf[p + 1] == 0 && buf[p + 2] == 1;

        buf[2] = 0x81;                                 /* response, recursion desired */
        buf[3] = 0x80;                                 /* recursion available, no error */
        buf[6] = 0;
        buf[7] = is_a ? 1 : 0;                         /* ANCOUNT */
        memset(buf + 8, 0, 4);                         /* NSCOUNT, ARCOUNT */
        int out = qend;
        if (is_a) {
            static const uint8_t ans[] = {0xC0, 0x0C, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4};
            memcpy(buf + out, ans, sizeof(ans));
            out += sizeof(ans);
            memcpy(buf + out, &ip.ip.addr, 4);         /* already network order */
            out += 4;
        }
        sendto(sock, buf, (size_t)out, 0, (struct sockaddr *)&from, flen);
    }
}

void dns_start(void) { xTaskCreate(dns_task, "cm_dns", 4096, NULL, 4, NULL); }
