idf_component_register(SRCS "access_point.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_wifi nvs_flash esp_http_server html wifi_manager json)