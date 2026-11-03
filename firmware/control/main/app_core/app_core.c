#include "app_core.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "audio_service.h"
#include "light_service.h"
#include "ui_service.h"
#include "hw_profile.h"
#include "sc_trace.h"

static const char *TAG = "sc_app_core";

static QueueHandle_t s_event_queue;
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
static bool s_debug_mode;
#endif

esp_err_t sc_app_core_post_event(const sc_app_event_t *event)
{
    if ((s_event_queue == NULL) || (event == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_event_queue, event, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void sc_app_core_task(void *arg)
{
    (void)arg;
    sc_app_event_t event;

    while (1) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        SC_TRACE_MARK("app_core", "evt_rx", (int32_t)event.type);
        SC_TRACE_MARK("app_core", "dispatch_start", (int32_t)event.type);

        switch (event.type) {
            case SC_APP_EVT_BOOT:
                ESP_LOGI(TAG, "boot event handled");
                break;
            case SC_APP_EVT_UI_AUDIO_TOGGLE:
                // Touch UI semantics: single-tap toggles music playback.
                if (sc_audio_service_toggle_playback() != ESP_OK) {
                    ESP_LOGW(TAG, "audio toggle unavailable or already pending");
                }
                break;
            case SC_APP_EVT_UI_AUDIO_STOP:
                if (sc_audio_service_set_playback(false) != ESP_OK) {
                    ESP_LOGW(TAG, "audio stop unavailable or already pending");
                }
                break;
            case SC_APP_EVT_UI_VOLUME_STEP:
                (void)sc_audio_service_change_volume((int)event.value);
                break;
            case SC_APP_EVT_UI_VOLUME_SET:
                (void)sc_audio_service_set_volume_percent((uint8_t)event.value);
                break;
            case SC_APP_EVT_UI_LIGHT_STEP:
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_light_service_change_brightness((int)event.value));
                break;
            case SC_APP_EVT_AUDIO_AVAILABILITY_CHANGED:
                if (event.value != 0) {
                    ESP_LOGI(TAG, "authoritative audio available");
                } else {
                    ESP_LOGW(TAG,
                             "authoritative audio unavailable; presentation neutral, lighting retained");
                }
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
                if (s_debug_mode) {
                    sc_ui_service_debug_refresh();
                }
#endif
                break;
            case SC_APP_EVT_AUDIO_STATE_CHANGED:
                sc_ui_service_audio_state_changed(event.value != 0);
                ESP_ERROR_CHECK_WITHOUT_ABORT(
                    sc_light_service_audio_sway(event.value != 0));
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
                if (s_debug_mode) {
                    sc_ui_service_debug_refresh();
                }
#endif
                break;
            case SC_APP_EVT_AUDIO_VOLUME_CHANGED:
                sc_ui_service_audio_volume_changed((uint8_t)event.value);
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
                if (s_debug_mode) {
                    sc_ui_service_debug_refresh();
                }
#endif
                break;
            case SC_APP_EVT_AUDIO_DEBUG_CONFIG_CHANGED:
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
                if (s_debug_mode) {
                    sc_ui_service_debug_refresh();
                }
#endif
                break;
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE
            case SC_APP_EVT_UI_DEBUG_ENTER:
                if (!s_debug_mode &&
                    sc_light_service_debug_enter_touch_mode() == ESP_OK) {
                    s_debug_mode = true;
                    sc_ui_service_debug_enter();
                }
                break;
            case SC_APP_EVT_UI_DEBUG_EXIT:
                if (s_debug_mode) {
                    s_debug_mode = false;
                    sc_audio_service_restore_debug_defaults();
                    ESP_ERROR_CHECK_WITHOUT_ABORT(
                        sc_light_service_debug_exit_touch_mode());
                    sc_ui_service_debug_exit();
                }
                break;
            case SC_APP_EVT_UI_DEBUG_VOLUME_NEXT: {
                const uint8_t current = sc_audio_service_get_volume_percent();
                const uint8_t next = current >= 100U
                                         ? 0U
                                         : (uint8_t)(((current / 10U) + 1U) * 10U);
                (void)sc_audio_service_set_volume_percent(next);
                sc_ui_service_debug_refresh();
                break;
            }
            case SC_APP_EVT_UI_DEBUG_LIGHT_TOGGLE:
                (void)sc_light_service_toggle();
                sc_ui_service_debug_refresh();
                break;
            case SC_APP_EVT_UI_DEBUG_DUTY_NEXT: {
                sc_light_debug_status_t status = {0};
                if (sc_light_service_debug_get_status(&status) == ESP_OK) {
                    const uint16_t next = status.manual_duty_bps >= 10000U
                                              ? 0U
                                              : (uint16_t)(((status.manual_duty_bps / 1000U) + 1U) * 1000U);
                    (void)sc_light_service_debug_set_pwm_duty_bps(next);
                }
                sc_ui_service_debug_refresh();
                break;
            }
            case SC_APP_EVT_UI_DEBUG_SOURCE_TOGGLE: {
                const sc_audio_source_t next =
                    sc_audio_service_get_source() == SC_AUDIO_SOURCE_TONE
                        ? SC_AUDIO_SOURCE_MUSIC : SC_AUDIO_SOURCE_TONE;
                (void)sc_audio_service_set_source(next);
                break;
            }
            case SC_APP_EVT_UI_DEBUG_FREQUENCY_NEXT: {
                sc_light_debug_status_t status = {0};
                if (sc_light_service_debug_get_status(&status) == ESP_OK) {
                    uint8_t current = (uint8_t)(status.requested_frequency_hz / 1000U);
                    const uint8_t next = current >= SC_LIGHT_DEBUG_PWM_MAX_KHZ
                                             ? SC_LIGHT_DEBUG_PWM_MIN_KHZ
                                             : (uint8_t)(current + 1U);
                    (void)sc_light_service_debug_set_pwm_frequency_khz(next);
                }
                sc_ui_service_debug_refresh();
                break;
            }
            case SC_APP_EVT_UI_DEBUG_NOTE_NEXT: {
                const sc_audio_tone_note_t current = sc_audio_service_get_tone_note();
                const sc_audio_tone_note_t next =
                    (sc_audio_tone_note_t)((current + 1U) % SC_AUDIO_TONE_COUNT);
                (void)sc_audio_service_set_tone_note(next);
                break;
            }
#endif
            default:
                break;
        }
        SC_TRACE_MARK("app_core", "dispatch_end", (int32_t)event.type);
    }
}

esp_err_t sc_app_core_start(void)
{
    sc_hw_profile_log();

    s_event_queue = xQueueCreate(8, sizeof(sc_app_event_t));
    if (s_event_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(sc_app_core_task, "sc_app_core", 4096, NULL, 6, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }

#if CONFIG_SC_ENABLE_LIGHT
    ESP_RETURN_ON_ERROR(sc_light_service_start(), TAG, "light service start failed");
#else
    ESP_LOGI(TAG, "light service disabled");
#endif

    ESP_RETURN_ON_ERROR(sc_audio_service_start(), TAG, "audio service start failed");

#if CONFIG_SC_ENABLE_UI
    ESP_RETURN_ON_ERROR(sc_ui_service_start(), TAG, "ui service start failed");
#else
    ESP_LOGI(TAG, "ui service disabled");
#endif

    sc_app_event_t boot_evt = { .type = SC_APP_EVT_BOOT, .value = 0 };
    (void)sc_app_core_post_event(&boot_evt);
    return ESP_OK;
}
