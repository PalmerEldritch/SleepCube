#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_light_service_start(void);
esp_err_t sc_light_service_set_enabled(bool enable);
esp_err_t sc_light_service_toggle(void);
esp_err_t sc_light_service_change_brightness(int delta_steps);
esp_err_t sc_light_service_audio_sway(bool audio_enabled);
bool sc_light_service_get_enabled(void);
uint8_t sc_light_service_get_current_brightness_percent(void);
uint8_t sc_light_service_get_target_brightness_percent(void);
