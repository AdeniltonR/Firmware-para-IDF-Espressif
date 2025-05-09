idf_component_register(SRCS "api_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES mbedtls esp_http_client json)
