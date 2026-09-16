idf_component_register(
    SRCS
        "main.c"
        "audio_output.c"
        "diagnostic_tone.c"
    INCLUDE_DIRS "."
    REQUIRES
        esp_app_format
        esp_driver_gpio
        esp_driver_i2s
        esp_system
        spi_flash
)
