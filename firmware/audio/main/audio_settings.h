#pragma once

#include <stdint.h>

#include "esp_err.h"

/** Initialize NVS and load a validated persistent volume percentage. */
esp_err_t sc_audio_settings_init(uint8_t *volume_percent);

/** Persist a validated volume percentage only when it has changed. */
esp_err_t sc_audio_settings_save_volume(uint8_t volume_percent);
