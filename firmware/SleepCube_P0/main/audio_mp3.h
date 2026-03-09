#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Decode and play an MP3 file over I2S.
 *
 * The file is decoded frame-by-frame and converted to interleaved 16-bit stereo PCM
 * (mono frames are duplicated to both channels).
 *
 * @param path Absolute VFS path to MP3 file (for example `/spiffs/test.mp3`).
 * @return
 * - ESP_OK when file playback finishes successfully
 * - ESP_ERR_NOT_FOUND if the file cannot be opened
 * - ESP_ERR_NO_MEM if decode buffers cannot be allocated
 * - Other error code if I2S write fails
 *
 * @note Playback I2S rate is currently fixed at 44.1 kHz.
 * @note @docready
 */
esp_err_t sc_audio_mp3_play_file(const char *path, const volatile bool *play_enabled, uint8_t volume_percent);
