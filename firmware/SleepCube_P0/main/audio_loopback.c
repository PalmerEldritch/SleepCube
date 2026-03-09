#include "audio_loopback.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "audio_i2s.h"

static const char *TAG = "sc_loopback";

#define SC_LOOPBACK_FRAMES_PER_READ  (256U)
#define SC_LOOPBACK_SIGNAL_THRESH    (64)

typedef struct {
    uint32_t sample_rate_hz;
} sc_loopback_task_arg_t;

static void sc_loopback_task(void *arg)
{
    const sc_loopback_task_arg_t *cfg = (const sc_loopback_task_arg_t *)arg;
    const uint32_t sample_rate_hz = cfg->sample_rate_hz;
    static int16_t rx_buf[SC_LOOPBACK_FRAMES_PER_READ * 2];

    uint32_t frames_acc = 0;
    uint32_t nonzero_acc = 0;
    uint32_t mismatch_acc = 0;
    uint32_t crossing_acc = 0;
    int16_t prev_l = 0;
    bool prev_loud = false;
    bool prev_valid = false;

    while (1) {
        size_t frames_read = 0;
        esp_err_t err = sc_audio_i2s_read(rx_buf, SC_LOOPBACK_FRAMES_PER_READ, &frames_read, 1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        for (size_t i = 0; i < frames_read; i++) {
            const int16_t l = rx_buf[(i * 2) + 0];
            const int16_t r = rx_buf[(i * 2) + 1];
            const bool loud = (((l >= SC_LOOPBACK_SIGNAL_THRESH) || (l <= -SC_LOOPBACK_SIGNAL_THRESH)) ||
                               ((r >= SC_LOOPBACK_SIGNAL_THRESH) || (r <= -SC_LOOPBACK_SIGNAL_THRESH)));

            if (loud) {
                nonzero_acc++;
            }
            if (l != r) {
                mismatch_acc++;
            }
            if (prev_valid && prev_loud && loud && (prev_l <= 0) && (l > 0)) {
                crossing_acc++;
            }
            prev_l = l;
            prev_loud = loud;
            prev_valid = true;
        }

        frames_acc += (uint32_t)frames_read;

        if (frames_acc >= sample_rate_hz) {
            const float signal_pct = (frames_acc > 0) ? (100.0f * (float)nonzero_acc / (float)frames_acc) : 0.0f;
            const float mismatch_pct = (frames_acc > 0) ? (100.0f * (float)mismatch_acc / (float)frames_acc) : 0.0f;
            float est_freq_hz = 0.0f;
            if (signal_pct > 5.0f && frames_acc > 0) {
                est_freq_hz = ((float)crossing_acc * (float)sample_rate_hz / (float)frames_acc);
            }

            ESP_LOGI(TAG, "loopback: signal=%.1f%% stereo_mismatch=%.2f%% est_freq=%.1f Hz",
                     signal_pct, mismatch_pct, est_freq_hz);

            frames_acc = 0;
            nonzero_acc = 0;
            mismatch_acc = 0;
            crossing_acc = 0;
        }
    }
}

esp_err_t sc_audio_loopback_start(uint32_t sample_rate_hz)
{
    ESP_RETURN_ON_ERROR(sc_audio_i2s_init_rx(sample_rate_hz), TAG, "I2S RX init failed");

    static sc_loopback_task_arg_t cfg;
    cfg.sample_rate_hz = sample_rate_hz;

    BaseType_t task_ok = xTaskCreate(
        sc_loopback_task,
        "sc_loopback",
        4096,
        &cfg,
        4,
        NULL
    );

    if (task_ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create loopback task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "loopback monitor started");
    return ESP_OK;
}
