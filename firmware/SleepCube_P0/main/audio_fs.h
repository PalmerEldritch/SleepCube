#pragma once

#include "esp_err.h"

/**
 * @brief Mount the SPIFFS partition used for audio assets.
 *
 * The partition label is fixed to `storage` and mounted on `/spiffs`.
 *
 * @return
 * - ESP_OK on success
 * - Error code from `esp_vfs_spiffs_register` or `esp_spiffs_info` on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_fs_mount(void);
