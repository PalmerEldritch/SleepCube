set(SC_AUDIO_SRCS
        "main.c"
        "audio_output.c"
        "audio_settings.c"
        "content_storage.c"
        "pcm_format.c"
        "playback_controller.c"
        "wav_format.c"
        "wav_reader.c"
)

if(CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO)
    list(APPEND SC_AUDIO_SRCS
        "audio_test_profiles.c"
        "tone_generator.c"
    )
endif()

idf_component_register(
    SRCS ${SC_AUDIO_SRCS}
    INCLUDE_DIRS "."
    REQUIRES
        esp_app_format
        esp_driver_gpio
        esp_driver_i2s
        esp_driver_sdspi
        esp_driver_spi
        esp_timer
        esp_system
        fatfs
        nvs_flash
        sdmmc
        spi_flash
        transport
)
