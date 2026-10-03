idf_component_register(SRCS "main.c" "portal.c" "dns.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES "index.html"
                       REQUIRES core esp_wifi esp_netif esp_event nvs_flash
                                esp_http_server esp_timer lwip)
