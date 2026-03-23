#pragma once

#include "esp_err.h"

/**
 * @brief Mount the available filesystem backends used for audio assets.
 *
 * Current behavior:
 * - always mounts the SPIFFS partition `storage` on `/spiffs`
 * - on supported boards, attempts to mount the SD card on `/sdcard`
 *
 * @return
 * - ESP_OK when the mandatory SPIFFS backend is mounted
 * - Error code from `esp_vfs_spiffs_register` or `esp_spiffs_info` on failure
 *
 * @note @docready
 */
esp_err_t sc_audio_fs_mount(void);

/**
 * @brief Resolve the preferred playback path for the default test MP3.
 *
 * Preference order:
 * 1) `/sdcard/test.mp3` if the SD card is mounted and the file exists
 * 2) `/spiffs/test.mp3`
 *
 * @return Absolute VFS path to the preferred test MP3.
 */
const char *sc_audio_fs_get_default_mp3_path(void);
