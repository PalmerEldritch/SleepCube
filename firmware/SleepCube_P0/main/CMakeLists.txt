idf_component_register(
    SRCS
        "main.c"
        "audio_i2s.c"
        "audio_player.c"
        "audio_loopback.c"
        "audio_fs.c"
        "audio_mp3.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_i2s
        driver
        spiffs
)

spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
