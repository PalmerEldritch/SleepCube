#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

typedef enum {
    SC_AUDIO_MP3_SOURCE_AUTO = 0,
    SC_AUDIO_MP3_SOURCE_SPIFFS = 1,
    SC_AUDIO_MP3_SOURCE_SD = 2,
} sc_audio_mp3_source_t;

#define SC_AUDIO_FS_MAX_TRACKS    16U
#define SC_AUDIO_FS_MAX_PATH_LEN  128U

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
esp_err_t sc_audio_fs_remount_sdcard(void);
bool sc_audio_fs_sd_mounted(void);

/**
 * @brief Resolve the preferred playback path for the default test MP3.
 *
 * Preference order:
 * 1) `/sdcard/test.mp3` if the SD card is mounted and the file exists
 * 2) `/spiffs/test.mp3`
 *
 * @return Absolute VFS path to the preferred test MP3.
 *
 * @note @docready
 */
const char *sc_audio_fs_get_default_mp3_path(void);
const char *sc_audio_fs_get_mp3_path(sc_audio_mp3_source_t source);
bool sc_audio_fs_mp3_source_available(sc_audio_mp3_source_t source);
size_t sc_audio_fs_list_sd_tracks(char tracks[][SC_AUDIO_FS_MAX_PATH_LEN], size_t max_tracks);
bool sc_audio_fs_resolve_sd_track(const char *selector, char *path, size_t path_len);
size_t sc_audio_fs_list_sd_entries(char entries[][SC_AUDIO_FS_MAX_PATH_LEN], size_t max_entries);
