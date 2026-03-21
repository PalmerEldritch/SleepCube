#include "audio_player.h"

#include <stddef.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "audio_fs.h"
#include "audio_i2s.h"
#include "audio_loopback.h"
#include "audio_mp3.h"
#include "audio_tone.h"
#include "sc_trace.h"

static const char *TAG = "sc_audio_player";

#define SC_SAMPLE_RATE_HZ  (44100U)
#define SC_MP3_PATH        ("/spiffs/test.mp3")
#define SC_PLAYER_TASK_STACK_WORDS (8192)
#define SC_AUDIO_TONE_DIAGNOSTIC (0)
#define SC_TONE_FREQUENCY_HZ (440.0f)
#define SC_TONE_AMPLITUDE    (0.18f)
#define SC_TONE_FRAMES       (1024U)
#define SC_I2S_WRITE_TIMEOUT_MS (200U)

static volatile bool s_play_enabled = false;
static volatile uint8_t s_volume_percent = 40;

static void sc_audio_player_task(void *arg)
{
    (void)arg;
    bool was_playing = false;
#if SC_AUDIO_TONE_DIAGNOSTIC
    static int16_t tone_buf[SC_TONE_FRAMES * 2];
    sc_tone_state_t tone;
    sc_tone_init(&tone, SC_SAMPLE_RATE_HZ, SC_TONE_FREQUENCY_HZ, SC_TONE_AMPLITUDE);
#endif

    while (1) {
        if (!s_play_enabled) {
            if (was_playing) {
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_i2s_set_tx_enabled(false));
                ESP_LOGI(TAG, "playback stopped, output muted");
                SC_TRACE_MARK("audio", "state_muted", 0);
                was_playing = false;
            }
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        SC_TRACE_MARK("audio", "play_start", s_volume_percent);
        ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_i2s_set_tx_enabled(true));
        was_playing = true;
#if SC_AUDIO_TONE_DIAGNOSTIC
        ESP_LOGI(TAG, "tone diagnostic active: %.0f Hz amp=%.2f volume=%u%% format=msb-32",
                 (double)SC_TONE_FREQUENCY_HZ, (double)SC_TONE_AMPLITUDE, (unsigned)s_volume_percent);
        esp_err_t err = ESP_OK;
        while (s_play_enabled) {
            for (size_t i = 0; i < SC_TONE_FRAMES; i++) {
                int16_t s = sc_tone_next_sample(&tone);
                s = (int16_t)(((int32_t)s * (int32_t)s_volume_percent) / 100);
                tone_buf[(i * 2U) + 0U] = s;
                tone_buf[(i * 2U) + 1U] = s;
            }
            err = sc_audio_i2s_write(tone_buf, SC_TONE_FRAMES, SC_I2S_WRITE_TIMEOUT_MS);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "tone i2s write failed: %s", esp_err_to_name(err));
                break;
            }
        }
#else
        esp_err_t err = sc_audio_mp3_play_file(SC_MP3_PATH, &s_play_enabled, s_volume_percent);
#endif
        SC_TRACE_MARK("audio", "play_end", err);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "playback failed: %s", esp_err_to_name(err));
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

    ESP_LOGI(TAG, "mp3 playback task started (%s), initial enabled=%d volume=%u%%",
             SC_MP3_PATH, s_play_enabled, (unsigned)s_volume_percent);
    return ESP_OK;
}

void sc_audio_player_set_enabled(bool enable)
{
    s_play_enabled = enable;
}

bool sc_audio_player_get_enabled(void)
{
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
