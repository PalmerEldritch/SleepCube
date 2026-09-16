#pragma once

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Play a PCM WAV file over I2S.
 *
 * Supported format:
 * - RIFF/WAVE
 * - PCM (format 1)
 * - 16-bit mono or stereo
 *
 * @param path Absolute VFS path to WAV file.
 * @param play_enabled Optional flag checked during playback to allow stop requests.
 * @return
 * - ESP_OK when playback finishes successfully
 * - ESP_ERR_NOT_FOUND if the file cannot be opened
 * - ESP_ERR_NOT_SUPPORTED for unsupported WAV formats
 * - Other error code if I2S write fails or parsing fails
 */
esp_err_t sc_audio_wav_play_file(const char *path, const volatile bool *play_enabled);
