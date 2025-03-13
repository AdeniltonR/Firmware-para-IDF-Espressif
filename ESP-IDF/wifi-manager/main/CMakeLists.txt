idf_component_register(
    SRCS "nvs_sync.c" "dns_server.c" "main.c" "wifi_manager.c" "json.c" "http_app.c"
    INCLUDE_DIRS "."
)

# Inclua os arquivos binários
target_add_binary_data(${COMPONENT_TARGET} "index.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "code.js" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "style.css" TEXT)