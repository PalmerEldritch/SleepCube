#include "audio_player.h"

#include <stddef.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "audio_fs.h"
#include "audio_hpf.h"
#include "audio_i2s.h"
#include "audio_loopback.h"
#include "audio_mp3.h"
#include "audio_tone.h"
#include "audio_wav.h"
#include "sc_trace.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>

static const char *TAG = "sc_audio_player";

#define SC_SAMPLE_RATE_HZ  (44100U)
#define SC_PLAYER_TASK_STACK_WORDS (8192)
#define SC_TONE_FRAMES       (1024U)
#define SC_I2S_WRITE_TIMEOUT_MS (200U)
#define SC_SLEEP_TIMER_DURATION_US (30ULL * 60ULL * 1000000ULL)
#define SC_STOP_FADE_DURATION_US   (500ULL * 1000ULL)
#define SC_TIMER_FADE_DURATION_US  (15ULL * 1000000ULL)

#if CONFIG_SC_AUDIO_SOURCE_DIAGNOSTIC
#if CONFIG_SC_AUDIO_DIAG_SIGNAL_SWEEP
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_SWEEP
#define SC_AUDIO_DEFAULT_TONE_HZ        500U
#elif CONFIG_SC_AUDIO_DIAG_SIGNAL_100HZ
#define SC_AUDIO_DEFAULT_TONE_HZ        100U
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_TONE
#elif CONFIG_SC_AUDIO_DIAG_SIGNAL_200HZ
#define SC_AUDIO_DEFAULT_TONE_HZ        200U
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_TONE
#elif CONFIG_SC_AUDIO_DIAG_SIGNAL_1000HZ
#define SC_AUDIO_DEFAULT_TONE_HZ        1000U
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_TONE
#else
#define SC_AUDIO_DEFAULT_TONE_HZ        500U
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_TONE
#endif
#else
#define SC_AUDIO_DEFAULT_SOURCE_MODE    SC_AUDIO_SOURCE_MODE_MP3
#define SC_AUDIO_DEFAULT_TONE_HZ        500U
#endif

#ifdef CONFIG_SC_AUDIO_DIAG_AMPLITUDE_PCT
#define SC_AUDIO_DEFAULT_TONE_AMPLITUDE_PCT  CONFIG_SC_AUDIO_DIAG_AMPLITUDE_PCT
#else
#define SC_AUDIO_DEFAULT_TONE_AMPLITUDE_PCT  18U
#endif

#ifdef CONFIG_SC_AUDIO_DIAG_SWEEP_START_HZ
#define SC_AUDIO_DEFAULT_SWEEP_START_HZ  CONFIG_SC_AUDIO_DIAG_SWEEP_START_HZ
#else
#define SC_AUDIO_DEFAULT_SWEEP_START_HZ  80U
#endif

#ifdef CONFIG_SC_AUDIO_DIAG_SWEEP_END_HZ
#define SC_AUDIO_DEFAULT_SWEEP_END_HZ  CONFIG_SC_AUDIO_DIAG_SWEEP_END_HZ
#else
#define SC_AUDIO_DEFAULT_SWEEP_END_HZ  1500U
#endif

#ifdef CONFIG_SC_AUDIO_DIAG_SWEEP_PERIOD_MS
#define SC_AUDIO_DEFAULT_SWEEP_PERIOD_MS  CONFIG_SC_AUDIO_DIAG_SWEEP_PERIOD_MS
#else
#define SC_AUDIO_DEFAULT_SWEEP_PERIOD_MS  5000U
#endif

static volatile bool s_play_enabled = false;
static volatile uint8_t s_volume_percent = 40;
static volatile uint8_t s_effective_volume_percent = 40;
static volatile uint64_t s_sleep_deadline_us = 0;
static volatile uint64_t s_fade_start_us = 0;
static volatile uint64_t s_fade_duration_us = 0;
static volatile bool s_fade_active = false;
static portMUX_TYPE s_audio_cfg_lock = portMUX_INITIALIZER_UNLOCKED;
static sc_audio_source_mode_t s_source_mode = SC_AUDIO_DEFAULT_SOURCE_MODE;
static uint16_t s_tone_frequency_hz = SC_AUDIO_DEFAULT_TONE_HZ;
static uint8_t s_tone_amplitude_percent = SC_AUDIO_DEFAULT_TONE_AMPLITUDE_PCT;
static uint16_t s_sweep_start_hz = SC_AUDIO_DEFAULT_SWEEP_START_HZ;
static uint16_t s_sweep_end_hz = SC_AUDIO_DEFAULT_SWEEP_END_HZ;
static uint16_t s_sweep_period_ms = SC_AUDIO_DEFAULT_SWEEP_PERIOD_MS;
static uint16_t s_hpf_cutoff_hz = CONFIG_SC_AUDIO_HPF_CUTOFF_HZ;
static uint8_t s_hpf_stages = CONFIG_SC_AUDIO_HPF_STAGES;
static uint16_t s_lpf_cutoff_hz = 0U;
static uint8_t s_lpf_stages = 1U;
static sc_audio_mp3_source_t s_mp3_source = SC_AUDIO_MP3_SOURCE_AUTO;
static sc_audio_mp3_mix_mode_t s_mp3_mix_mode = SC_AUDIO_MP3_MIX_STEREO;
static sc_audio_mp3_eq_mode_t s_mp3_eq_mode = SC_AUDIO_MP3_EQ_OFF;
static uint8_t s_mp3_pre_gain_db = 0U;
static bool s_mp3_limiter_enabled = false;
static uint8_t s_mp3_limiter_threshold_pct = 85U;
static uint16_t s_mp3_eq_cutoff_hz = 3500U;
static uint8_t s_mp3_eq_depth_pct = 50U;
static uint32_t s_mp3_decode_error_count = 0;
static uint32_t s_mp3_sync_miss_count = 0;
static uint32_t s_mp3_rate_mismatch_count = 0;
static char s_mp3_path[SC_AUDIO_FS_MAX_PATH_LEN] = "";

static uint64_t sc_audio_player_now_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

static void sc_audio_player_begin_sleep_timer(void)
{
    s_sleep_deadline_us = sc_audio_player_now_us() + SC_SLEEP_TIMER_DURATION_US;
    s_fade_active = false;
    s_fade_start_us = 0;
    s_fade_duration_us = 0;
    s_effective_volume_percent = s_volume_percent;
}

static void sc_audio_player_begin_fadeout(uint64_t duration_us)
{
    if (!s_play_enabled) {
        return;
    }
    if (!s_fade_active) {
        s_fade_active = true;
        s_fade_start_us = sc_audio_player_now_us();
        s_fade_duration_us = duration_us;
    }
}

static void sc_audio_player_refresh_state(void)
{
    if (!s_play_enabled) {
        s_effective_volume_percent = s_volume_percent;
        return;
    }

    const uint64_t now_us = sc_audio_player_now_us();
    if (!s_fade_active && (s_sleep_deadline_us != 0) && (now_us >= s_sleep_deadline_us)) {
        ESP_LOGI(TAG, "sleep timer expired, starting fade-out");
        sc_audio_player_begin_fadeout(SC_TIMER_FADE_DURATION_US);
    }

    if (!s_fade_active) {
        s_effective_volume_percent = s_volume_percent;
        return;
    }

    const uint64_t elapsed_us = now_us - s_fade_start_us;
    if ((s_fade_duration_us == 0) || (elapsed_us >= s_fade_duration_us)) {
        s_effective_volume_percent = 0;
        s_play_enabled = false;
        s_fade_active = false;
        s_sleep_deadline_us = 0;
        return;
    }

    const uint64_t remaining_us = s_fade_duration_us - elapsed_us;
    s_effective_volume_percent = (uint8_t)(((uint32_t)s_volume_percent * (uint32_t)remaining_us) / (uint32_t)s_fade_duration_us);
}

static void sc_audio_player_get_diag_config(sc_audio_source_mode_t *source_mode,
                                            uint16_t *tone_hz,
                                            uint8_t *amplitude_percent,
                                            uint16_t *sweep_start_hz,
                                            uint16_t *sweep_end_hz,
                                            uint16_t *sweep_period_ms,
                                            uint16_t *hpf_cutoff_hz,
                                            uint8_t *hpf_stages,
                                            uint16_t *lpf_cutoff_hz,
                                            uint8_t *lpf_stages)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    if (source_mode != NULL) {
        *source_mode = s_source_mode;
    }
    if (tone_hz != NULL) {
        *tone_hz = s_tone_frequency_hz;
    }
    if (amplitude_percent != NULL) {
        *amplitude_percent = s_tone_amplitude_percent;
    }
    if (sweep_start_hz != NULL) {
        *sweep_start_hz = s_sweep_start_hz;
    }
    if (sweep_end_hz != NULL) {
        *sweep_end_hz = s_sweep_end_hz;
    }
    if (sweep_period_ms != NULL) {
        *sweep_period_ms = s_sweep_period_ms;
    }
    if (hpf_cutoff_hz != NULL) {
        *hpf_cutoff_hz = s_hpf_cutoff_hz;
    }
    if (hpf_stages != NULL) {
        *hpf_stages = s_hpf_stages;
    }
    if (lpf_cutoff_hz != NULL) {
        *lpf_cutoff_hz = s_lpf_cutoff_hz;
    }
    if (lpf_stages != NULL) {
        *lpf_stages = s_lpf_stages;
    }
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

static void sc_audio_player_get_selected_mp3_path(char *path, size_t path_len)
{
    if ((path == NULL) || (path_len == 0U)) {
        return;
    }

    taskENTER_CRITICAL(&s_audio_cfg_lock);
    if (s_mp3_path[0] != '\0') {
        (void)snprintf(path, path_len, "%s", s_mp3_path);
    } else {
        (void)snprintf(path, path_len, "%s", sc_audio_fs_get_mp3_path(s_mp3_source));
    }
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

static float sc_audio_diag_frequency_hz(sc_audio_source_mode_t source_mode,
                                        uint16_t tone_hz,
                                        uint16_t sweep_start_hz,
                                        uint16_t sweep_end_hz,
                                        uint16_t sweep_period_ms,
                                        uint64_t now_us)
{
    if (source_mode != SC_AUDIO_SOURCE_MODE_SWEEP) {
        return (float)tone_hz;
    }

    const uint64_t period_us = (uint64_t)sweep_period_ms * 1000ULL;
    const uint64_t pos_us = (period_us > 0) ? (now_us % period_us) : 0;
    const float t = (period_us > 0) ? ((float)pos_us / (float)period_us) : 0.0f;
    return (float)sweep_start_hz + (((float)sweep_end_hz - (float)sweep_start_hz) * t);
}

static const char *sc_audio_diag_signal_name(sc_audio_source_mode_t source_mode)
{
    return (source_mode == SC_AUDIO_SOURCE_MODE_SWEEP) ? "sweep" : "tone";
}

static bool sc_audio_player_is_wav_path(const char *path)
{
    if (path == NULL) {
        return false;
    }

    const char *ext = strrchr(path, '.');
    return (ext != NULL) && (strcasecmp(ext, ".wav") == 0);
}

static void sc_audio_player_task(void *arg)
{
    (void)arg;
    bool was_playing = false;
    static int16_t tone_buf[SC_TONE_FRAMES * 2];
    sc_tone_state_t tone;
    sc_hpf_chain_t tone_left_hpf = { 0 };
    sc_lpf_chain_t tone_left_lpf = { 0 };
    sc_audio_source_mode_t source_mode = SC_AUDIO_DEFAULT_SOURCE_MODE;
    uint16_t tone_hz = SC_AUDIO_DEFAULT_TONE_HZ;
    uint8_t amplitude_percent = SC_AUDIO_DEFAULT_TONE_AMPLITUDE_PCT;
    uint16_t sweep_start_hz = SC_AUDIO_DEFAULT_SWEEP_START_HZ;
    uint16_t sweep_end_hz = SC_AUDIO_DEFAULT_SWEEP_END_HZ;
    uint16_t sweep_period_ms = SC_AUDIO_DEFAULT_SWEEP_PERIOD_MS;
    uint16_t hpf_cutoff_hz = CONFIG_SC_AUDIO_HPF_CUTOFF_HZ;
    uint8_t hpf_stages = CONFIG_SC_AUDIO_HPF_STAGES;
    uint16_t lpf_cutoff_hz = 0U;
    uint8_t lpf_stages = 1U;
    sc_tone_init(&tone, SC_SAMPLE_RATE_HZ, (float)tone_hz, (float)amplitude_percent / 100.0f);

    while (1) {
        if (!s_play_enabled) {
            if (was_playing) {
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_i2s_set_tx_enabled(false));
                ESP_LOGI(TAG, "playback stopped, output muted");
                SC_TRACE_MARK("audio", "state_muted", 0);
                was_playing = false;
                sc_audio_hpf_reset(&tone_left_hpf);
                sc_audio_lpf_chain_reset(&tone_left_lpf);
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        sc_audio_player_refresh_state();
        sc_audio_player_get_diag_config(&source_mode, &tone_hz, &amplitude_percent,
                                        &sweep_start_hz, &sweep_end_hz, &sweep_period_ms,
                                        &hpf_cutoff_hz, &hpf_stages,
                                        &lpf_cutoff_hz, &lpf_stages);
        SC_TRACE_MARK("audio", "play_start", s_effective_volume_percent);
        ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_i2s_set_tx_enabled(true));
        was_playing = true;
        if (source_mode != SC_AUDIO_SOURCE_MODE_MP3) {
            ESP_LOGI(TAG, "audio diagnostic active: signal=%s amp=%u%% volume=%u%%",
                     sc_audio_diag_signal_name(source_mode),
                     (unsigned)amplitude_percent,
                     (unsigned)s_volume_percent);
            esp_err_t err = ESP_OK;
            while (s_play_enabled) {
                float freq_hz = 0.0f;
                uint16_t hpf_alpha_q15 = 0U;
                uint16_t lpf_alpha_q15 = 0U;
                sc_audio_player_refresh_state();
                sc_audio_player_get_diag_config(&source_mode, &tone_hz, &amplitude_percent,
                                                &sweep_start_hz, &sweep_end_hz, &sweep_period_ms,
                                                &hpf_cutoff_hz, &hpf_stages,
                                                &lpf_cutoff_hz, &lpf_stages);
                tone.amplitude = (float)amplitude_percent / 100.0f;
                freq_hz = sc_audio_diag_frequency_hz(source_mode, tone_hz, sweep_start_hz,
                                                     sweep_end_hz, sweep_period_ms,
                                                     sc_audio_player_now_us());
                sc_tone_set_frequency(&tone, SC_SAMPLE_RATE_HZ, freq_hz);
                hpf_alpha_q15 = sc_audio_hpf_alpha_q15(SC_SAMPLE_RATE_HZ, hpf_cutoff_hz);
                lpf_alpha_q15 = sc_audio_lpf_alpha_q15(SC_SAMPLE_RATE_HZ, lpf_cutoff_hz);

                for (size_t i = 0; i < SC_TONE_FRAMES; i++) {
                    int16_t s = sc_tone_next_sample(&tone);
                    if (sc_audio_hpf_enabled(hpf_cutoff_hz, hpf_stages)) {
                        s = sc_audio_hpf_process_q15(&tone_left_hpf, s, hpf_alpha_q15, hpf_stages);
                    }
                    if (sc_audio_lpf_enabled(lpf_cutoff_hz, lpf_stages)) {
                        s = sc_audio_lpf_chain_process_q15(&tone_left_lpf, s, lpf_alpha_q15, lpf_stages);
                    }
                    s = (int16_t)(((int32_t)s * (int32_t)s_effective_volume_percent) / 100);
                    tone_buf[(i * 2U) + 0U] = s;
                    tone_buf[(i * 2U) + 1U] = s;
                }
                err = sc_audio_i2s_write(tone_buf, SC_TONE_FRAMES, SC_I2S_WRITE_TIMEOUT_MS);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "tone i2s write failed: %s", esp_err_to_name(err));
                    break;
                }
            }
            sc_audio_hpf_reset(&tone_left_hpf);
            sc_audio_lpf_chain_reset(&tone_left_lpf);
            SC_TRACE_MARK("audio", "play_end", err);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "playback failed: %s", esp_err_to_name(err));
            }
        } else {
            char mp3_path[SC_AUDIO_FS_MAX_PATH_LEN];
            sc_audio_player_get_selected_mp3_path(mp3_path, sizeof(mp3_path));
            esp_err_t err = sc_audio_player_is_wav_path(mp3_path) ?
                            sc_audio_wav_play_file(mp3_path, &s_play_enabled) :
                            sc_audio_mp3_play_file(mp3_path, &s_play_enabled);
            SC_TRACE_MARK("audio", "play_end", err);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "playback failed: %s", esp_err_to_name(err));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t sc_audio_player_start(void)
{
    ESP_RETURN_ON_ERROR(sc_audio_fs_mount(), TAG, "SPIFFS mount failed");

#if CONFIG_SC_LOOPBACK_ENABLE
    ESP_RETURN_ON_ERROR(sc_audio_loopback_start(SC_SAMPLE_RATE_HZ), TAG, "Loopback start failed");
    ESP_LOGI(TAG, "loopback monitor: enabled");
#else
    ESP_LOGI(TAG, "loopback monitor: disabled");
#endif

    ESP_RETURN_ON_ERROR(sc_audio_i2s_init(SC_SAMPLE_RATE_HZ), TAG, "I2S init failed");

    if (sc_audio_hpf_enabled(s_hpf_cutoff_hz, s_hpf_stages)) {
        ESP_LOGI(TAG, "audio HPF config: cutoff=%u Hz stages=%u",
                 (unsigned)s_hpf_cutoff_hz, (unsigned)s_hpf_stages);
    } else {
        ESP_LOGI(TAG, "audio HPF config: disabled");
    }

    if (s_source_mode == SC_AUDIO_SOURCE_MODE_MP3) {
        char mp3_path[SC_AUDIO_FS_MAX_PATH_LEN];
        sc_audio_player_get_selected_mp3_path(mp3_path, sizeof(mp3_path));
        ESP_LOGI(TAG, "audio source: mp3 playback (%s)",
                 (s_mp3_source == SC_AUDIO_MP3_SOURCE_SPIFFS) ? "spiffs" :
                 (s_mp3_source == SC_AUDIO_MP3_SOURCE_SD) ? "sd" : "auto");
        ESP_LOGI(TAG, "audio mp3 config: mix=%s pre_gain=-%u dB limiter=%s@%u%% eq=%s/%uHz/%u%%",
                 (s_mp3_mix_mode == SC_AUDIO_MP3_MIX_MONO) ? "mono" :
                 (s_mp3_mix_mode == SC_AUDIO_MP3_MIX_LEFT) ? "left" :
                 (s_mp3_mix_mode == SC_AUDIO_MP3_MIX_RIGHT) ? "right" : "stereo",
                 (unsigned)s_mp3_pre_gain_db,
                 s_mp3_limiter_enabled ? "on" : "off",
                 (unsigned)s_mp3_limiter_threshold_pct,
                 (s_mp3_eq_mode == SC_AUDIO_MP3_EQ_LPF) ? "lpf" :
                 (s_mp3_eq_mode == SC_AUDIO_MP3_EQ_PRESENCE_CUT) ? "presence" : "off",
                 (unsigned)s_mp3_eq_cutoff_hz,
                 (unsigned)s_mp3_eq_depth_pct);
        ESP_LOGI(TAG, "audio track: %s", mp3_path);
    } else if (s_source_mode == SC_AUDIO_SOURCE_MODE_SWEEP) {
        ESP_LOGI(TAG, "audio source: diagnostic (%s %u-%u Hz over %u ms)",
                 sc_audio_diag_signal_name(s_source_mode),
                 (unsigned)s_sweep_start_hz,
                 (unsigned)s_sweep_end_hz,
                 (unsigned)s_sweep_period_ms);
    } else {
        ESP_LOGI(TAG, "audio source: diagnostic (%u Hz tone amp=%u%%)",
                 (unsigned)s_tone_frequency_hz,
                 (unsigned)s_tone_amplitude_percent);
    }

    BaseType_t task_ok = xTaskCreate(
        sc_audio_player_task,
        "sc_audio_player",
        SC_PLAYER_TASK_STACK_WORDS,
        NULL,
        5,
        NULL
    );

    if (task_ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create audio task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "audio task started (preferred path=%s), initial enabled=%d volume=%u%%",
             sc_audio_fs_get_default_mp3_path(), s_play_enabled, (unsigned)s_volume_percent);
    return ESP_OK;
}

void sc_audio_player_set_enabled(bool enable)
{
    if (enable) {
        s_play_enabled = true;
        sc_audio_player_begin_sleep_timer();
        return;
    }

    s_play_enabled = false;
    s_sleep_deadline_us = 0;
    s_fade_active = false;
    s_fade_start_us = 0;
    s_fade_duration_us = 0;
    s_effective_volume_percent = s_volume_percent;
}

void sc_audio_player_request_stop(void)
{
    sc_audio_player_begin_fadeout(SC_STOP_FADE_DURATION_US);
}

bool sc_audio_player_get_enabled(void)
{
    sc_audio_player_refresh_state();
    return s_play_enabled;
}

void sc_audio_player_set_volume_percent(uint8_t volume_percent)
{
    if (volume_percent > 100U) {
        volume_percent = 100U;
    }
    s_volume_percent = volume_percent;
}

uint8_t sc_audio_player_get_volume_percent(void)
{
    return s_volume_percent;
}

uint8_t sc_audio_player_get_effective_volume_percent(void)
{
    sc_audio_player_refresh_state();
    return s_effective_volume_percent;
}

void sc_audio_player_set_source_mode(sc_audio_source_mode_t mode)
{
    if ((mode != SC_AUDIO_SOURCE_MODE_MP3) &&
        (mode != SC_AUDIO_SOURCE_MODE_TONE) &&
        (mode != SC_AUDIO_SOURCE_MODE_SWEEP)) {
        return;
    }

    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_source_mode = mode;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

sc_audio_source_mode_t sc_audio_player_get_source_mode(void)
{
    sc_audio_source_mode_t mode;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    mode = s_source_mode;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return mode;
}

void sc_audio_player_set_tone_frequency_hz(uint16_t frequency_hz)
{
    if (frequency_hz == 0U) {
        frequency_hz = 1U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_tone_frequency_hz = frequency_hz;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

uint16_t sc_audio_player_get_tone_frequency_hz(void)
{
    uint16_t frequency_hz;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    frequency_hz = s_tone_frequency_hz;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return frequency_hz;
}

void sc_audio_player_set_tone_amplitude_percent(uint8_t amplitude_percent)
{
    if (amplitude_percent > 100U) {
        amplitude_percent = 100U;
    }
    if (amplitude_percent == 0U) {
        amplitude_percent = 1U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_tone_amplitude_percent = amplitude_percent;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

uint8_t sc_audio_player_get_tone_amplitude_percent(void)
{
    uint8_t amplitude_percent;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    amplitude_percent = s_tone_amplitude_percent;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return amplitude_percent;
}

void sc_audio_player_set_sweep(uint16_t start_hz, uint16_t end_hz, uint16_t period_ms)
{
    if (start_hz == 0U) {
        start_hz = 1U;
    }
    if (end_hz < start_hz) {
        uint16_t tmp = start_hz;
        start_hz = end_hz;
        end_hz = tmp;
    }
    if (end_hz == 0U) {
        end_hz = start_hz;
    }
    if (period_ms == 0U) {
        period_ms = 1000U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_sweep_start_hz = start_hz;
    s_sweep_end_hz = end_hz;
    s_sweep_period_ms = period_ms;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_get_sweep(uint16_t *start_hz, uint16_t *end_hz, uint16_t *period_ms)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    if (start_hz != NULL) {
        *start_hz = s_sweep_start_hz;
    }
    if (end_hz != NULL) {
        *end_hz = s_sweep_end_hz;
    }
    if (period_ms != NULL) {
        *period_ms = s_sweep_period_ms;
    }
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_set_hpf(uint16_t cutoff_hz, uint8_t stages)
{
    if (stages > SC_AUDIO_HPF_MAX_STAGES) {
        stages = SC_AUDIO_HPF_MAX_STAGES;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_hpf_cutoff_hz = cutoff_hz;
    s_hpf_stages = stages;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_set_lpf(uint16_t cutoff_hz, uint8_t stages)
{
    if (stages > SC_AUDIO_LPF_MAX_STAGES) {
        stages = SC_AUDIO_LPF_MAX_STAGES;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_lpf_cutoff_hz = cutoff_hz;
    s_lpf_stages = stages;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

uint16_t sc_audio_player_get_hpf_cutoff_hz(void)
{
    uint16_t cutoff_hz;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    cutoff_hz = s_hpf_cutoff_hz;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return cutoff_hz;
}

uint8_t sc_audio_player_get_hpf_stages(void)
{
    uint8_t stages;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    stages = s_hpf_stages;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return stages;
}

uint16_t sc_audio_player_get_lpf_cutoff_hz(void)
{
    uint16_t cutoff_hz;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    cutoff_hz = s_lpf_cutoff_hz;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return cutoff_hz;
}

uint8_t sc_audio_player_get_lpf_stages(void)
{
    uint8_t stages;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    stages = s_lpf_stages;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return stages;
}

void sc_audio_player_set_mp3_source(sc_audio_mp3_source_t source)
{
    if ((source != SC_AUDIO_MP3_SOURCE_AUTO) &&
        (source != SC_AUDIO_MP3_SOURCE_SPIFFS) &&
        (source != SC_AUDIO_MP3_SOURCE_SD)) {
        return;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_source = source;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

sc_audio_mp3_source_t sc_audio_player_get_mp3_source(void)
{
    sc_audio_mp3_source_t source;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    source = s_mp3_source;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return source;
}

void sc_audio_player_set_mp3_mix_mode(sc_audio_mp3_mix_mode_t mode)
{
    if ((mode != SC_AUDIO_MP3_MIX_STEREO) &&
        (mode != SC_AUDIO_MP3_MIX_MONO) &&
        (mode != SC_AUDIO_MP3_MIX_LEFT) &&
        (mode != SC_AUDIO_MP3_MIX_RIGHT)) {
        return;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_mix_mode = mode;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

sc_audio_mp3_mix_mode_t sc_audio_player_get_mp3_mix_mode(void)
{
    sc_audio_mp3_mix_mode_t mode;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    mode = s_mp3_mix_mode;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return mode;
}

void sc_audio_player_set_mp3_pre_gain_db(uint8_t db)
{
    if (db > 24U) {
        db = 24U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_pre_gain_db = db;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

uint8_t sc_audio_player_get_mp3_pre_gain_db(void)
{
    uint8_t db;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    db = s_mp3_pre_gain_db;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return db;
}

void sc_audio_player_set_mp3_limiter(bool enable, uint8_t threshold_pct)
{
    if (threshold_pct < 50U) {
        threshold_pct = 50U;
    }
    if (threshold_pct > 99U) {
        threshold_pct = 99U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_limiter_enabled = enable;
    s_mp3_limiter_threshold_pct = threshold_pct;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

bool sc_audio_player_get_mp3_limiter_enabled(void)
{
    bool enabled;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    enabled = s_mp3_limiter_enabled;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return enabled;
}

uint8_t sc_audio_player_get_mp3_limiter_threshold_pct(void)
{
    uint8_t threshold_pct;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    threshold_pct = s_mp3_limiter_threshold_pct;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return threshold_pct;
}

void sc_audio_player_set_mp3_eq(sc_audio_mp3_eq_mode_t mode, uint16_t cutoff_hz, uint8_t depth_pct)
{
    if ((mode != SC_AUDIO_MP3_EQ_OFF) &&
        (mode != SC_AUDIO_MP3_EQ_LPF) &&
        (mode != SC_AUDIO_MP3_EQ_PRESENCE_CUT)) {
        return;
    }
    if (cutoff_hz == 0U) {
        cutoff_hz = 3500U;
    }
    if (depth_pct > 100U) {
        depth_pct = 100U;
    }
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_eq_mode = mode;
    s_mp3_eq_cutoff_hz = cutoff_hz;
    s_mp3_eq_depth_pct = depth_pct;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

sc_audio_mp3_eq_mode_t sc_audio_player_get_mp3_eq_mode(void)
{
    sc_audio_mp3_eq_mode_t mode;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    mode = s_mp3_eq_mode;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return mode;
}

uint16_t sc_audio_player_get_mp3_eq_cutoff_hz(void)
{
    uint16_t cutoff_hz;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    cutoff_hz = s_mp3_eq_cutoff_hz;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return cutoff_hz;
}

uint8_t sc_audio_player_get_mp3_eq_depth_pct(void)
{
    uint8_t depth_pct;
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    depth_pct = s_mp3_eq_depth_pct;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return depth_pct;
}

bool sc_audio_player_set_mp3_path(const char *path)
{
    if ((path == NULL) || (path[0] == '\0')) {
        return false;
    }

    taskENTER_CRITICAL(&s_audio_cfg_lock);
    int written = snprintf(s_mp3_path, sizeof(s_mp3_path), "%s", path);
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    return (written > 0) && ((size_t)written < sizeof(s_mp3_path));
}

void sc_audio_player_get_mp3_path(char *path, size_t path_len)
{
    sc_audio_player_get_selected_mp3_path(path, path_len);
}

void sc_audio_player_reset_mp3_stats(void)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_decode_error_count = 0;
    s_mp3_sync_miss_count = 0;
    s_mp3_rate_mismatch_count = 0;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_note_mp3_decode_error(void)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_decode_error_count++;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_note_mp3_sync_miss(void)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_sync_miss_count++;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_note_mp3_rate_mismatch(void)
{
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    s_mp3_rate_mismatch_count++;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
}

void sc_audio_player_get_runtime_config(sc_audio_runtime_config_t *config)
{
    if (config == NULL) {
        return;
    }

    sc_audio_player_refresh_state();
    taskENTER_CRITICAL(&s_audio_cfg_lock);
    config->source_mode = s_source_mode;
    config->mp3_source = s_mp3_source;
    config->mp3_mix_mode = s_mp3_mix_mode;
    config->mp3_eq_mode = s_mp3_eq_mode;
    (void)snprintf(config->mp3_path, sizeof(config->mp3_path), "%s",
                   (s_mp3_path[0] != '\0') ? s_mp3_path : sc_audio_fs_get_mp3_path(s_mp3_source));
    config->mp3_pre_gain_db = s_mp3_pre_gain_db;
    config->mp3_limiter_enabled = s_mp3_limiter_enabled;
    config->mp3_limiter_threshold_pct = s_mp3_limiter_threshold_pct;
    config->mp3_eq_cutoff_hz = s_mp3_eq_cutoff_hz;
    config->mp3_eq_depth_pct = s_mp3_eq_depth_pct;
    config->tone_frequency_hz = s_tone_frequency_hz;
    config->tone_amplitude_percent = s_tone_amplitude_percent;
    config->sweep_start_hz = s_sweep_start_hz;
    config->sweep_end_hz = s_sweep_end_hz;
    config->sweep_period_ms = s_sweep_period_ms;
    config->hpf_cutoff_hz = s_hpf_cutoff_hz;
    config->hpf_stages = s_hpf_stages;
    config->lpf_cutoff_hz = s_lpf_cutoff_hz;
    config->lpf_stages = s_lpf_stages;
    config->mp3_decode_error_count = s_mp3_decode_error_count;
    config->mp3_sync_miss_count = s_mp3_sync_miss_count;
    config->mp3_rate_mismatch_count = s_mp3_rate_mismatch_count;
    taskEXIT_CRITICAL(&s_audio_cfg_lock);
    config->volume_percent = s_volume_percent;
    config->playback_enabled = s_play_enabled;
}
