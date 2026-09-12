#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize I2S TX channel for audio playback.
 *
 * Configures the menuconfig-selected TX bus mode, framing, board-specific TX
 * pins, and playback sample format.
 *
 * Supported transmit formats:
 * - 44.1 kHz sample rate
 * - 16-bit PCM data width
 * - 32-bit slot width
 * - Standard I2S stereo or 8-slot TDM
 * - Philips I2S or MSB framing
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
 * @brief Enable or disable the TX channel without changing configuration.
 *
 * @param enable True to enable TX, false to disable TX.
 * @return
 * - ESP_OK on success
 * - Error code from I2S driver control functions on failure
 */
esp_err_t sc_audio_i2s_set_tx_enabled(bool enable);

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
 * @param timeout_ms Timeout in milliseconds passed directly to the I2S driver.
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
