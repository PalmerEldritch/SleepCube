idf_component_register(
    SRCS "sc_m3_transport.c"
    INCLUDE_DIRS "."
    REQUIRES protocol esp_driver_uart esp_timer
)
