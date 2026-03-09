#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t sc_light_service_start(void);
esp_err_t sc_light_service_set_enabled(bool enable);
esp_err_t sc_light_service_toggle(void);
esp_err_t sc_light_service_change_brightness(int delta_steps);
esp_err_t sc_light_service_audio_sway(bool audio_enabled);
