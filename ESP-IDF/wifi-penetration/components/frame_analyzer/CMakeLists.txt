idf_component_register(
    SRCS "frame_analyzer.c"  # Substitua pelo nome do arquivo fonte do seu componente
    INCLUDE_DIRS "interface"
    REQUIRES esp_event wifi_controller frame_analyzer
)