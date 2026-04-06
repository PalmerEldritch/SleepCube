#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "audio_fs.h"
#include "audio_hpf.h"

typedef enum {
    SC_AUDIO_SOURCE_MODE_MP3 = 0,
    SC_AUDIO_SOURCE_MODE_TONE = 1,
    SC_AUDIO_SOURCE_MODE_SWEEP = 2,
} sc_audio_source_mode_t;

typedef enum {
    SC_AUDIO_MP3_MIX_STEREO = 0,
    SC_AUDIO_MP3_MIX_MONO = 1,
    SC_AUDIO_MP3_MIX_LEFT = 2,
    SC_AUDIO_MP3_MIX_RIGHT = 3,
} sc_audio_mp3_mix_mode_t;

typedef enum {
    SC_AUDIO_MP3_EQ_OFF = 0,
    SC_AUDIO_MP3_EQ_LPF = 1,
    SC_AUDIO_MP3_EQ_PRESENCE_CUT = 2,
} sc_audio_mp3_eq_mode_t;

typedef struct {
    sc_audio_source_mode_t source_mode;
    sc_audio_mp3_source_t mp3_source;
    sc_audio_mp3_mix_mode_t mp3_mix_mode;
    sc_audio_mp3_eq_mode_t mp3_eq_mode;
    char mp3_path[SC_AUDIO_FS_MAX_PATH_LEN];
    uint8_t mp3_pre_gain_db;
    bool mp3_limiter_enabled;
    uint8_t mp3_limiter_threshold_pct;
    uint16_t mp3_eq_cutoff_hz;
    uint8_t mp3_eq_depth_pct;
    uint16_t tone_frequency_hz;
    uint8_t tone_amplitude_percent;
    uint16_t sweep_start_hz;
    uint16_t sweep_end_hz;
    uint16_t sweep_period_ms;
    uint16_t hpf_cutoff_hz;
    uint8_t hpf_stages;
    uint8_t volume_percent;
    bool playback_enabled;
    uint32_t mp3_decode_error_count;
    uint32_t mp3_sync_miss_count;
    uint32_t mp3_rate_mismatch_count;
} sc_audio_runtime_config_t;

/**
 * @brief Start the audio playback subsystem.
 *
 * Startup sequence:
 * 1) Mount SPIFFS.
 * 2) Optionally start loopback monitor depending on `CONFIG_SC_LOOPBACK_ENABLE`.
 * 3) Initialize I2S TX.
 * 4) Start playback task for the preferred MP3 path.
 *
 * @return
 * - ESP_OK on success
 * - Error code if any initialization step fails
 *
 * @note @docready
 */
esp_err_t sc_audio_player_start(void);
void sc_audio_player_set_enabled(bool enable);
void sc_audio_player_request_stop(void);
bool sc_audio_player_get_enabled(void);
void sc_audio_player_set_volume_percent(uint8_t volume_percent);
uint8_t sc_audio_player_get_volume_percent(void);
uint8_t sc_audio_player_get_effective_volume_percent(void);
void sc_audio_player_set_source_mode(sc_audio_source_mode_t mode);
sc_audio_source_mode_t sc_audio_player_get_source_mode(void);
void sc_audio_player_set_tone_frequency_hz(uint16_t frequency_hz);
uint16_t sc_audio_player_get_tone_frequency_hz(void);
void sc_audio_player_set_tone_amplitude_percent(uint8_t amplitude_percent);
uint8_t sc_audio_player_get_tone_amplitude_percent(void);
void sc_audio_player_set_sweep(uint16_t start_hz, uint16_t end_hz, uint16_t period_ms);
void sc_audio_player_get_sweep(uint16_t *start_hz, uint16_t *end_hz, uint16_t *period_ms);
void sc_audio_player_set_hpf(uint16_t cutoff_hz, uint8_t stages);
uint16_t sc_audio_player_get_hpf_cutoff_hz(void);
uint8_t sc_audio_player_get_hpf_stages(void);
void sc_audio_player_set_mp3_source(sc_audio_mp3_source_t source);
sc_audio_mp3_source_t sc_audio_player_get_mp3_source(void);
void sc_audio_player_set_mp3_mix_mode(sc_audio_mp3_mix_mode_t mode);
sc_audio_mp3_mix_mode_t sc_audio_player_get_mp3_mix_mode(void);
void sc_audio_player_set_mp3_pre_gain_db(uint8_t db);
uint8_t sc_audio_player_get_mp3_pre_gain_db(void);
void sc_audio_player_set_mp3_limiter(bool enable, uint8_t threshold_pct);
bool sc_audio_player_get_mp3_limiter_enabled(void);
uint8_t sc_audio_player_get_mp3_limiter_threshold_pct(void);
void sc_audio_player_set_mp3_eq(sc_audio_mp3_eq_mode_t mode, uint16_t cutoff_hz, uint8_t depth_pct);
sc_audio_mp3_eq_mode_t sc_audio_player_get_mp3_eq_mode(void);
uint16_t sc_audio_player_get_mp3_eq_cutoff_hz(void);
uint8_t sc_audio_player_get_mp3_eq_depth_pct(void);
bool sc_audio_player_set_mp3_path(const char *path);
void sc_audio_player_get_mp3_path(char *path, size_t path_len);
void sc_audio_player_reset_mp3_stats(void);
void sc_audio_player_get_runtime_config(sc_audio_runtime_config_t *config);
void sc_audio_player_note_mp3_decode_error(void);
void sc_audio_player_note_mp3_sync_miss(void);
void sc_audio_player_note_mp3_rate_mismatch(void);
