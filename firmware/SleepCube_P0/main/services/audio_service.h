#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_audio_service_start(void);
esp_err_t sc_audio_service_set_playback(bool enable);
esp_err_t sc_audio_service_toggle_playback(void);
esp_err_t sc_audio_service_change_volume(int delta_steps);
bool sc_audio_service_get_playback_enabled(void);
uint8_t sc_audio_service_get_volume_percent(void);
