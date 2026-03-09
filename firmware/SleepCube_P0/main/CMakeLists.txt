set(SC_SRCS
    "main.c"
    "app_core/app_core.c"
    "services/audio_service.c"
    "services/light_service.c"
    "services/ui_service.c"
    "hal/hw_profile.c"
    "trace/sc_trace.c"
)

if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_INPUT_BUTTONS)
    list(APPEND SC_SRCS "services/ui_input_buttons.c")
endif()

if(CONFIG_SC_ENABLE_AUDIO)
    list(APPEND SC_SRCS
        "audio_i2s.c"
        "audio_player.c"
        "audio_fs.c"
        "audio_mp3.c"
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

set(SC_PRIV_REQUIRES esp_driver_i2s driver spiffs esp_timer)
if(CONFIG_SC_ENABLE_LIGHT AND CONFIG_SC_LIGHT_BACKEND_LED_STRIP)
    list(APPEND SC_PRIV_REQUIRES esp_driver_rmt)
endif()

idf_component_register(
    SRCS ${SC_SRCS}
    INCLUDE_DIRS "." "app_core" "services" "hal" "drivers" "lighting" "trace"
    PRIV_REQUIRES ${SC_PRIV_REQUIRES}
)

spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
