#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_output.h"
#include "board_config.h"
#include "diagnostic_tone.h"

static const char *TAG = "m1_bringup";

static esp_err_t configure_button(void)
{
    const gpio_config_t button_config = {
        .pin_bit_mask = 1ULL << SC_BOARD_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&button_config);
}

static void log_platform_identity(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    uint32_t flash_size = 0;
    const esp_err_t flash_result = esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "SleepCube Audio Controller M1 diagnostic");
    ESP_LOGI(TAG, "app=%s version=%s idf=%s target=esp32s3 reset_reason=%d",
             app->project_name,
             app->version,
             esp_get_idf_version(),
             (int)esp_reset_reason());
    if (flash_result == ESP_OK) {
        ESP_LOGI(TAG, "flash=%" PRIu32 " bytes (%" PRIu32 " MiB)",
                 flash_size, flash_size / (1024U * 1024U));
    } else {
        ESP_LOGW(TAG, "flash size query failed: %s", esp_err_to_name(flash_result));
    }
    ESP_LOGI(TAG,
             "i2s format=%s rate=%" PRIu32 "Hz bits=%u dma=%ux%u "
             "bclk=GPIO%d lrclk=GPIO%d data=GPIO%d mclk=unused",
             SC_AUDIO_FORMAT_NAME,
             SC_AUDIO_SAMPLE_RATE_HZ,
             SC_AUDIO_BITS_PER_SAMPLE,
             SC_AUDIO_DMA_DESC_NUM,
             SC_AUDIO_DMA_FRAME_NUM,
             SC_AUDIO_I2S_BCLK_GPIO,
             SC_AUDIO_I2S_LRCLK_GPIO,
             SC_AUDIO_I2S_DATA_GPIO);
    ESP_LOGI(TAG, "button=GPIO%d active-low; startup playback is disabled",
             SC_BOARD_BUTTON_GPIO);
}

void app_main(void)
{
    /* Allow the native USB Serial/JTAG endpoint to re-enumerate after reset. */
    vTaskDelay(pdMS_TO_TICKS(SC_STARTUP_LOG_DELAY_MS));
    log_platform_identity();

    esp_err_t result = configure_button();
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "button initialization failed: %s; audio will remain disabled",
                 esp_err_to_name(result));
        return;
    }

    sc_audio_output_t output = {0};
    result = sc_audio_output_init(&output);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "audio initialization failed: %s; output remains disabled",
                 esp_err_to_name(result));
        sc_audio_output_fail_safe(&output);
        return;
    }

    ESP_LOGI(TAG, "READY: output is silent; press the button once to play the finite diagnostic sequence");

    bool previous_pressed = gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0;
    uint32_t sequence_number = 0;

    for (;;) {
        const bool pressed = gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0;
        if (pressed && !previous_pressed) {
            vTaskDelay(pdMS_TO_TICKS(SC_BUTTON_DEBOUNCE_MS));
            if (gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0) {
                sequence_number++;
                ESP_LOGI(TAG, "sequence=%" PRIu32 " start", sequence_number);

                size_t frames_written = 0;
                result = sc_diagnostic_tone_play(&output, &frames_written);
                if (result != ESP_OK) {
                    ESP_LOGE(TAG,
                             "sequence=%" PRIu32 " failed after %u frames: %s; "
                             "latching silent fault state",
                             sequence_number,
                             (unsigned)frames_written,
                             esp_err_to_name(result));
                    sc_audio_output_fail_safe(&output);
                    return;
                }

                ESP_LOGI(TAG,
                         "sequence=%" PRIu32 " complete frames=%u bytes=%u",
                         sequence_number,
                         (unsigned)frames_written,
                         (unsigned)(frames_written * SC_AUDIO_BYTES_PER_FRAME));
            }
        }

        previous_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(SC_BUTTON_POLL_MS));
    }
}
