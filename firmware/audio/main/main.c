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
#include "esp_random.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "playback_controller.h"
#include "sc_audio_link.h"
#include "wav_reader.h"

static const char *TAG = "m4_audio";
static sc_playback_controller_t s_controller;
static volatile bool s_controller_ready;
static volatile uint16_t s_startup_fault_code;

_Static_assert(SC_PLAYBACK_STOPPED + 1 == SC_AUDIO_STATE_STOPPED &&
                   SC_PLAYBACK_STARTING + 1 == SC_AUDIO_STATE_STARTING &&
                   SC_PLAYBACK_PLAYING + 1 == SC_AUDIO_STATE_PLAYING &&
                   SC_PLAYBACK_STOPPING + 1 == SC_AUDIO_STATE_STOPPING &&
                   SC_PLAYBACK_FAULT + 1 == SC_AUDIO_STATE_FAULT,
               "playback and wire state mappings must remain aligned");
_Static_assert((int)SC_PLAYBACK_STOP_REASON_BOOT ==
                       (int)SC_AUDIO_STOP_REASON_BOOT &&
                   (int)SC_PLAYBACK_STOP_REASON_MANUAL ==
                       (int)SC_AUDIO_STOP_REASON_MANUAL &&
                   (int)SC_PLAYBACK_STOP_REASON_TIMER_EXPIRED ==
                       (int)SC_AUDIO_STOP_REASON_TIMER_EXPIRED &&
                   (int)SC_PLAYBACK_STOP_REASON_FAULT ==
                       (int)SC_AUDIO_STOP_REASON_FAULT,
               "playback and wire stop-reason mappings must remain aligned");

static sc_audio_result_t link_get_status(void *context,
                                         sc_audio_status_t *status)
{
    (void)context;
    if (s_startup_fault_code != 0U) {
        status->state = SC_AUDIO_STATE_FAULT;
        status->stop_reason = SC_AUDIO_STOP_REASON_FAULT;
        status->fault_code = s_startup_fault_code;
        return SC_AUDIO_RESULT_ACCEPTED;
    }
    if (!s_controller_ready) {
        status->state = SC_AUDIO_STATE_NOT_READY;
        status->stop_reason = SC_AUDIO_STOP_REASON_BOOT;
        return SC_AUDIO_RESULT_ACCEPTED;
    }

    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    status->state = (sc_audio_state_t)snapshot.state + 1;
    status->volume_percent = snapshot.volume_percent;
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    status->flags = sc_audio_debug_status_flags(snapshot.source,
                                                snapshot.tone_note,
                                                snapshot.i2s_packing);
#endif
    status->stop_reason = (sc_audio_stop_reason_t)snapshot.stop_reason;
    status->remaining_seconds = snapshot.remaining_seconds;
    status->fault_code = snapshot.fault_code;
    return SC_AUDIO_RESULT_ACCEPTED;
}

static sc_audio_result_t link_play(void *context, uint32_t duration_seconds)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.state != SC_PLAYBACK_STOPPED) {
        return SC_AUDIO_RESULT_BUSY;
    }
    return sc_playback_controller_play(&s_controller, duration_seconds) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}

static sc_audio_result_t link_stop(void *context)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.state == SC_PLAYBACK_STOPPED ||
        snapshot.state == SC_PLAYBACK_STOPPING) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    return sc_playback_controller_stop(&s_controller) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}

static sc_audio_result_t link_set_volume(void *context, uint8_t volume_percent)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.volume_percent == volume_percent) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    return sc_playback_controller_set_volume(&s_controller, volume_percent) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}

#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
static sc_audio_result_t link_set_source(void *context,
                                         sc_audio_source_t source)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.source == source) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    return sc_playback_controller_set_source(&s_controller, source) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}

static sc_audio_result_t link_set_tone_note(void *context,
                                            sc_audio_tone_note_t note)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.tone_note == note) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    return sc_playback_controller_set_tone_note(&s_controller, note) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}

static sc_audio_result_t link_set_i2s_packing(
    void *context,
    sc_audio_i2s_packing_t packing)
{
    (void)context;
    if (!s_controller_ready) {
        return SC_AUDIO_RESULT_NOT_READY;
    }
    sc_playback_snapshot_t snapshot;
    if (sc_playback_controller_get_snapshot(&s_controller, &snapshot) != ESP_OK) {
        return SC_AUDIO_RESULT_INTERNAL_ERROR;
    }
    if (snapshot.i2s_packing == packing) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    return sc_playback_controller_set_i2s_packing(&s_controller, packing) == ESP_OK
               ? SC_AUDIO_RESULT_ACCEPTED
               : SC_AUDIO_RESULT_INTERNAL_ERROR;
}
#endif

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
    s_startup_fault_code = (uint16_t)error;
    ESP_LOGE(TAG, "%s failed: %s; startup remains silent and playback is disabled",
             operation, esp_err_to_name(error));
}

void app_main(void)
{
    /* Allow the native USB Serial/JTAG endpoint to re-enumerate after reset. */
    vTaskDelay(pdMS_TO_TICKS(SC_STARTUP_LOG_DELAY_MS));
    log_platform_identity();

    const sc_audio_backend_t link_backend = {
        .get_status = link_get_status,
        .play = link_play,
        .stop = link_stop,
        .set_volume = link_set_volume,
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
        .set_source = link_set_source,
        .set_tone_note = link_set_tone_note,
        .set_i2s_packing = link_set_i2s_packing,
#endif
    };
    uint32_t audio_session_id = esp_random();
    if (audio_session_id == 0U) {
        audio_session_id = 1U;
    }
    const esp_err_t link_result = sc_audio_link_start_audio(
        1, 2, audio_session_id,
        SC_SESSION_DURATION_MIN_SECONDS,
        SC_SESSION_DURATION_MAX_SECONDS,
        &link_backend);
    if (link_result != ESP_OK) {
        ESP_LOGE(TAG, "semantic UART unavailable: %s; audio remains standalone",
                 esp_err_to_name(link_result));
    }

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

    result = sc_playback_controller_init(&s_controller,
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
    s_controller_ready = true;

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
                result = sc_playback_controller_send(&s_controller, command);
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
