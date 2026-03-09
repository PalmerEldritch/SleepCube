#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t sc_audio_service_start(void);
esp_err_t sc_audio_service_set_playback(bool enable);
esp_err_t sc_audio_service_toggle_playback(void);
esp_err_t sc_audio_service_change_volume(int delta_steps);
