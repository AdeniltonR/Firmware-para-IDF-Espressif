idf_component_register(SRCS "emonlib-esp-idf.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_adc esp_timer)
