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

static const char *TAG = "sc_audio_player";

#define SC_SAMPLE_RATE_HZ  (44100U)
#define SC_MP3_PATH        ("/spiffs/test.mp3")
#define SC_PLAYER_TASK_STACK_WORDS (8192)

static void sc_audio_player_task(void *arg)
{
    (void)arg;

    while (1) {
        esp_err_t err = sc_audio_mp3_play_file(SC_MP3_PATH);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "playback failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(500));
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

    ESP_LOGI(TAG, "mp3 playback started from %s", SC_MP3_PATH);
    return ESP_OK;
}
