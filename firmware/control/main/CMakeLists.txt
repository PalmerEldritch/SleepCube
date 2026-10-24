set(SC_SRCS
    "main.c"
    "app_core/app_core.c"
    "services/audio_service.c"
    "services/light_service.c"
    "services/ui_service.c"
    "drivers/lvgl_display_if.c"
    "hal/hw_profile.c"
    "hal/settings_policy.c"
    "hal/settings_store.c"
    "trace/sc_trace.c"
)

idf_build_get_property(SC_IDF_PATH IDF_PATH)

set(SC_PRIV_INCLUDE_DIRS)

if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_INPUT_BUTTONS)
    list(APPEND SC_SRCS "services/ui_input_buttons.c")
endif()

if(CONFIG_SC_ENABLE_LIGHT)
    list(APPEND SC_SRCS
        "drivers/cob_pwm_if.c"
        "lighting/light_pwm_policy.c"
    )
endif()

if(CONFIG_SC_ENABLE_LIGHT_DEBUG_CONSOLE)
    list(APPEND SC_SRCS
        "debug/light_debug_command.c"
        "debug/light_debug_console.c"
    )
endif()

if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_BACKEND_LCD_TOUCH)
    list(APPEND SC_SRCS
        "drivers/lcd_panel_if.c"
    )
endif()

set(SC_REQUIRES esp_driver_gpio esp_lcd espressif__esp_lcd_touch espressif__esp_lvgl_port)
set(SC_PRIV_REQUIRES
    esp_driver_ledc
    esp_lcd_jd9853
    esp_lcd_touch_axs5106
    esp_timer
    esp_driver_spi
    nvs_flash
    transport
)
if(CONFIG_SC_ENABLE_UI AND CONFIG_SC_UI_BACKEND_LCD_TOUCH)
    list(APPEND SC_PRIV_REQUIRES
        esp_driver_spi
        esp_driver_i2c
    )
endif()

idf_component_register(
    SRCS ${SC_SRCS}
    INCLUDE_DIRS "." "app_core" "services" "hal" "drivers" "lighting" "trace" "debug"
    PRIV_INCLUDE_DIRS ${SC_PRIV_INCLUDE_DIRS}
    REQUIRES ${SC_REQUIRES}
    PRIV_REQUIRES ${SC_PRIV_REQUIRES}
)
