idf_component_register(
    SRCS "sc_audio_link.c"
    INCLUDE_DIRS "."
    REQUIRES protocol esp_driver_uart esp_timer esp_system
)
