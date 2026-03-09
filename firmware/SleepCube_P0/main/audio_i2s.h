#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize I2S TX channel for audio playback.
 *
 * Configures standard mode, 16-bit stereo slot format, and board-specific TX pins.
 *
 * @param sample_rate_hz I2S sample rate in Hz.
 * @return
 * - ESP_OK on success
 * - Error code from I2S driver init functions on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_i2s_init(uint32_t sample_rate_hz);

/**
 * @brief Initialize I2S RX channel for digital loopback monitoring.
 *
 * RX is configured as slave and expects BCLK/WS from the TX side via jumpers.
 *
 * @param sample_rate_hz Expected sample rate in Hz.
 * @return
 * - ESP_OK on success
 * - Error code from I2S driver init functions on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_i2s_init_rx(uint32_t sample_rate_hz);

/**
 * @brief Write interleaved stereo PCM frames to I2S TX.
 *
 * @param stereo_pcm Pointer to interleaved PCM frame data (L,R,L,R...).
 * @param frame_count Number of stereo frames to write.
 * @param timeout_ms Timeout in milliseconds.
 * @return
 * - ESP_OK if all requested frames were written
 * - ESP_ERR_TIMEOUT if a partial write occurred
 * - Other error code from `i2s_channel_write` on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_i2s_write(const int16_t *stereo_pcm, size_t frame_count, uint32_t timeout_ms);

/**
 * @brief Read interleaved stereo PCM frames from I2S RX.
 *
 * @param stereo_pcm Output buffer for interleaved PCM frame data.
 * @param frame_count Requested number of stereo frames to read.
 * @param frames_read Optional pointer receiving the number of frames actually read.
 * @param timeout_ms Timeout in milliseconds.
 * @return
 * - ESP_OK on success
 * - Error code from `i2s_channel_read` on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_i2s_read(int16_t *stereo_pcm, size_t frame_count, size_t *frames_read, uint32_t timeout_ms);
