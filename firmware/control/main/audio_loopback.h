#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Start the I2S loopback monitor task.
 *
 * The monitor reads looped-back RX data and logs signal presence, L/R mismatch,
 * and estimated frequency.
 *
 * @param sample_rate_hz Expected sample rate in Hz.
 * @return
 * - ESP_OK on success
 * - Error code if RX init or task creation fails
 *
 * @note @docready
 */
esp_err_t sc_audio_loopback_start(uint32_t sample_rate_hz);
