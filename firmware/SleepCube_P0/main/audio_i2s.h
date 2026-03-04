#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_audio_i2s_init(uint32_t sample_rate_hz);
esp_err_t sc_audio_i2s_init_rx(uint32_t sample_rate_hz);
esp_err_t sc_audio_i2s_write(const int16_t *stereo_pcm, size_t frame_count, uint32_t timeout_ms);
esp_err_t sc_audio_i2s_read(int16_t *stereo_pcm, size_t frame_count, size_t *frames_read, uint32_t timeout_ms);
