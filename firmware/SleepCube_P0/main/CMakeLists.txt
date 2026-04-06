set(SC_SRCS
    "main.c"
    "app_core/app_core.c"
    "services/audio_service.c"
    "services/light_service.c"
    "services/ui_service.c"
    "hal/hw_profile.c"
    "hal/settings_store.c"
    "trace/sc_trace.c"
)

idf_build_get_property(SC_IDF_PATH IDF_PATH)

set(SC_PRIV_INCLUDE_DIRS)

if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_INPUT_BUTTONS)
    list(APPEND SC_SRCS "audio_wav.c" "services/ui_input_buttons.c")
endif()

if(CONFIG_SC_ENABLE_AUDIO)
    list(APPEND SC_SRCS
        "audio_diag_console.c"
        "audio_hpf.c"
        "audio_i2s.c"
        "audio_player.c"
        "audio_tone.c"
        "audio_fs.c"
        "audio_mp3.c"
        "audio_wav.c"
    )
endif()

if(CONFIG_SC_ENABLE_AUDIO AND CONFIG_SC_LOOPBACK_ENABLE)
    list(APPEND SC_SRCS "audio_loopback.c")
endif()

if(CONFIG_SC_ENABLE_LIGHT AND CONFIG_SC_LIGHT_BACKEND_LED_STRIP)
    list(APPEND SC_SRCS
        "drivers/led_strip_encoder.c"
        "drivers/led_strip_if.c"
        "lighting/light_engine.c"
    )
endif()

if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_BACKEND_LCD_TOUCH)
    list(APPEND SC_SRCS
        "drivers/lcd_panel_if.c"
        "drivers/lvgl_display_if.c"
    )
endif()

set(SC_REQUIRES driver esp_lcd espressif__esp_lcd_touch espressif__esp_lvgl_port)
set(SC_PRIV_REQUIRES
    esp_driver_i2s
    esp_driver_ledc
    esp_lcd_jd9853
    esp_lcd_touch_axs5106
    spiffs
    esp_timer
    fatfs
    sdmmc
    esp_driver_spi
    nvs_flash
)
if(CONFIG_SC_ENABLE_LIGHT AND CONFIG_SC_LIGHT_BACKEND_LED_STRIP)
    list(APPEND SC_PRIV_REQUIRES esp_driver_rmt)
endif()
if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_BACKEND_LCD_TOUCH)
    list(APPEND SC_PRIV_REQUIRES
        esp_driver_spi
        esp_driver_i2c
    )
endif()

idf_component_register(
    SRCS ${SC_SRCS}
    INCLUDE_DIRS "." "app_core" "services" "hal" "drivers" "lighting" "trace"
    PRIV_INCLUDE_DIRS ${SC_PRIV_INCLUDE_DIRS}
    REQUIRES ${SC_REQUIRES}
    PRIV_REQUIRES ${SC_PRIV_REQUIRES}
)

if(CONFIG_SC_ENABLE_AUDIO)
    spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
endif()
