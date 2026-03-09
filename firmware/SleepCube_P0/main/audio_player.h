#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Start the audio playback subsystem.
 *
 * Startup sequence:
 * 1) Mount SPIFFS.
 * 2) Optionally start loopback monitor depending on `CONFIG_SC_LOOPBACK_ENABLE`.
 * 3) Initialize I2S TX.
 * 4) Start playback task that loops `/spiffs/test.mp3`.
 *
 * @return
 * - ESP_OK on success
 * - Error code if any initialization step fails
 *
 * @note @docready
 */
esp_err_t sc_audio_player_start(void);
void sc_audio_player_set_enabled(bool enable);
bool sc_audio_player_get_enabled(void);
void sc_audio_player_set_volume_percent(uint8_t volume_percent);
uint8_t sc_audio_player_get_volume_percent(void);
