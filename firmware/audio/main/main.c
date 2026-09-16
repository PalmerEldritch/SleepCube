#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "audio_output.h"
#include "audio_settings.h"
#include "board_config.h"
#include "content_storage.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "playback_controller.h"
#include "wav_reader.h"

static const char *TAG = "m2_audio";

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

    ESP_LOGI(TAG, "SleepCube Audio Controller %s", SC_BUILD_MODE_NAME);
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
    ESP_LOGI(TAG,
             "content=%s format=RIFF/WAVE PCM mono signed-16 44100Hz "
             "sd_clk=GPIO%d sd_mosi=GPIO%d sd_miso=GPIO%d sd_cs=fixed-low",
             SC_CONTENT_FILE_PATH,
             SC_SDCARD_CLK_GPIO,
             SC_SDCARD_MOSI_GPIO,
             SC_SDCARD_MISO_GPIO);
}

static void log_startup_failure(const char *operation, esp_err_t error)
{
    ESP_LOGE(TAG, "%s failed: %s; startup remains silent and playback is disabled",
             operation, esp_err_to_name(error));
}

void app_main(void)
{
    /* Allow the native USB Serial/JTAG endpoint to re-enumerate after reset. */
    vTaskDelay(pdMS_TO_TICKS(SC_STARTUP_LOG_DELAY_MS));
    log_platform_identity();

    esp_err_t result = configure_button();
    if (result != ESP_OK) {
        log_startup_failure("button initialization", result);
        return;
    }

    uint8_t initial_volume = 0;
    result = sc_audio_settings_init(&initial_volume);
    if (result != ESP_OK) {
        log_startup_failure("settings initialization", result);
        return;
    }

    sc_content_storage_t storage = {0};
    result = sc_content_storage_mount(&storage);
    if (result != ESP_OK) {
        log_startup_failure("content storage initialization", result);
        return;
    }

    sc_wav_reader_t reader = {0};
    result = sc_wav_reader_open(&reader, SC_CONTENT_FILE_PATH);
    if (result != ESP_OK) {
        log_startup_failure("content validation", result);
        sc_content_storage_unmount(&storage);
        return;
    }

    sc_audio_output_t output = {0};
    result = sc_audio_output_init(&output);
    if (result != ESP_OK) {
        log_startup_failure("audio output initialization", result);
        sc_wav_reader_close(&reader);
        sc_content_storage_unmount(&storage);
        return;
    }

    sc_playback_controller_t controller = {0};
    result = sc_playback_controller_init(&controller,
                                         &output,
                                         &reader,
                                         initial_volume);
    if (result != ESP_OK) {
        log_startup_failure("playback controller initialization", result);
        sc_audio_output_fail_safe(&output);
        sc_wav_reader_close(&reader);
        sc_content_storage_unmount(&storage);
        return;
    }

    ESP_LOGI(TAG,
             "READY: state=STOPPED and output=silent; short press toggles PLAY/STOP; "
             "hold for at least %ums then release to cycle persistent volume",
             SC_BUTTON_LONG_PRESS_MS);

    bool previous_pressed = gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0;
    TickType_t press_started = xTaskGetTickCount();

    for (;;) {
        const bool pressed = gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0;
        if (pressed && !previous_pressed) {
            vTaskDelay(pdMS_TO_TICKS(SC_BUTTON_DEBOUNCE_MS));
            if (gpio_get_level(SC_BOARD_BUTTON_GPIO) == 0) {
                press_started = xTaskGetTickCount();
                previous_pressed = true;
                continue;
            }
        } else if (!pressed && previous_pressed) {
            vTaskDelay(pdMS_TO_TICKS(SC_BUTTON_DEBOUNCE_MS));
            if (gpio_get_level(SC_BOARD_BUTTON_GPIO) != 0) {
                const uint32_t held_ms =
                    (uint32_t)((xTaskGetTickCount() - press_started) * portTICK_PERIOD_MS);
                const sc_playback_command_type_t command =
                    held_ms >= SC_BUTTON_LONG_PRESS_MS
                        ? SC_PLAYBACK_COMMAND_CYCLE_VOLUME
                        : SC_PLAYBACK_COMMAND_TOGGLE;
                result = sc_playback_controller_send(&controller, command);
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "button command rejected: %s",
                             esp_err_to_name(result));
                }
                previous_pressed = false;
                continue;
            }
        }

        previous_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(SC_BUTTON_POLL_MS));
    }
}
